
#include "Engine/render/PC/Graphic.h"
#ifdef WIN32
#include "D3dRenderer.h"
#include "Engine/common/Exception.h"
#include "Engine/render/RenderItem.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Private/D3dContext.h"
#include "Engine/render/PC/Resource/D3dAllocator.h"
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#include "Engine/render/PC/Resource/HlslShader.h"
#include "Engine/render/PC/Resource/RenderTexture.h"

#undef max
#undef min

Renderer* Renderer::sRenderer()
{
    return D3dRenderer::sD3dRenderer();
}

Renderer::~Renderer() = default;

// SampleRenderPath* gSampleRenderPath()
// {
//     static SampleRenderPath sampleRenderPath{};
//     return &sampleRenderPath;
// }

D3dRenderer* D3dRenderer::sD3dRenderer()
{
    static D3dRenderer renderer{};
    return &renderer;
}

uint8_t D3dRenderer::sCpuWorkingPageIdx()
{
    return sD3dRenderer()->mCpuWorkingPageIdx;
}

bool D3dRenderer::sIsRenderingThread()
{
    return std::this_thread::get_id() == sD3dRenderer()->mRenderingThreadId;
}

uint64_t D3dRenderer::sCalcPassCbvStartIdx(uint8_t cpuWorkingPageIdx)
{
    return cpuWorkingPageIdx * GraphicSetting::gNumPassConstants;
}

uint64_t D3dRenderer::sCalcObjectCbvOffset(uint8_t cpuWorkingPageIdx)
{
    static uint64_t objectDescsPerPage = GraphicSetting::gMaxRenderItemsPerFrame * GraphicSetting::gNumPerObjectConstants +
        GraphicSetting::gNumPassConstants * GraphicSetting::gNumBackBuffers;
    return cpuWorkingPageIdx * objectDescsPerPage;
}

void D3dRenderer::initialize(RendererSetting setting)
{
    mRenderingThreadId = std::this_thread::get_id();

    // ------------------mConfiguration---------------------
    mBackBufferFormat = setting.mBackBufferFormat;
    mDepthStencilFormat = setting.mDepthStencilFormat;
    mNumBackBuffers = setting.mNumBackBuffers;
    mMsaaSampleCount = setting.mMsaaSampleCount;
    mMsaaSampleQuality = setting.mMsaaSampleQuality;
    // ----------------------------------------------------
    
    D3dContext& context = D3dContext::instance();

    mMainGraphicQueue = context.createCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    mMainComputeQueue = context.createCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    mCopyQueue = context.createCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);

    mGraphicFence = std::move(context.createFence(0));
    mCopyFence = std::move(context.createFence(0));
    mGraphicContext.reset(mMainGraphicQueue.Get());
    mCopyContext.reset(mCopyQueue.Get());
    mFrameFenceValue = 0;
    mCopyFenceValue = 0;

    // create swapchain and backbuffers
    mSwapChain.Attach(
        context.createSwapChainForHwnd(mMainGraphicQueue.Get(), GraphicSetting::gBackBufferFormat,
                                            GraphicSetting::gNumBackBuffers)
            );
    DXGI_SWAP_CHAIN_DESC1 desc;
    mSwapChain->GetDesc1(&desc);

    createRootSignature();

    mRtDescHeap.reset(context.createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        GraphicSetting::gMaxNumRenderTarget));
    mDsDescHeap.reset(context.createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        GraphicSetting::gMaxNumRenderTarget));
    mCbSrUaDescHeap.reset(context.createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        GraphicSetting::gNumBackBuffers * (GraphicSetting::gNumPassConstants + GraphicSetting::gMaxRenderItemsPerFrame * GraphicSetting::gNumPerObjectConstants)));

    // TODO: 
    D3D12_CPU_DESCRIPTOR_HANDLE descHandle = mRtDescHeap->cpuHandle(0);
    mBackBuffers = new StaticHeap[GraphicSetting::gNumBackBuffers];
    for (int i = 0; i < GraphicSetting::gNumBackBuffers; ++i)
    {
        ID3D12Resource* backBuffer;
        ThrowIfFailed(
            mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer))
        );
        context.deviceHandle()->CreateRenderTargetView(backBuffer, nullptr, descHandle);
        descHandle.ptr += mRtDescHeap->descriptorSize();
        mBackBuffers[i] = { { backBuffer, 1, static_cast<ResourceState>(D3D12_RESOURCE_STATE_COMMON) } };
    }
    mCpuWorkingPageIdx = 0;
    // create depth stencil buffer
    mDepthStencilBuffer = D3D12RHIFactory::allocDepthStencilResource(desc.Width, desc.Height, GraphicSetting::gDepthStencilFormat);
    context.deviceHandle()->CreateDepthStencilView(mDepthStencilBuffer.nativePtr(), nullptr, mDsDescHeap->cpuHandle(0));


    D3dCommandListPool::initialize(mNumBackBuffers);

    mReleasingResources = new std::vector<uint64_t>[mNumBackBuffers + 1];
    mCBuffers = new std::vector<DynamicHeap>[mNumBackBuffers];
}

void D3dRenderer::updateDynamicResource(const ResourceHandle& resourceHandle, const void* data) const
{
    DynamicHeap* stagingBuffer = dynamic_cast<DynamicHeap*>(mResources[resourceHandle.mIndex]);
    memcpy(stagingBuffer->mappedPointer(), data, stagingBuffer->size());
}

void D3dRenderer::updateStaticResource(const ResourceHandle& resourceHandle, const void* data) const
{
    D3D12CommandList* pCommandList = D3dCommandListPool::getCommandList(CommandListType::COPY);
    StaticHeap* staticBuffer = dynamic_cast<StaticHeap*>(mResources[resourceHandle.mIndex]);
    uint64_t size = staticBuffer->size(); 
    DynamicHeap&& stagingBuffer = D3D12RHIFactory::allocDynamicBuffer(size);
    memcpy(stagingBuffer.mappedPointer(), data, size);
    pCommandList->copyResource(*staticBuffer, stagingBuffer);
    pCommandList->close();
    mCopyContext.executeCommandList(pCommandList);
    D3dCommandListPool::recycle(pCommandList);
}

void D3dRenderer::createRootSignature()
{
    // CD3DX12_DESCRIPTOR_RANGE1 range;
    // range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0);
    // CD3DX12_ROOT_PARAMETER1 rootParam;
    // rootParam.InitAsDescriptorTable(1, &range);
    // CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    // desc.Init_1_1(1, &rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    //
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges1;
    std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges2;
    std::vector<CD3DX12_ROOT_PARAMETER1> rootParams;
    
    // TODO: remove magic number
    ranges1.resize(1);
    ranges2.resize(1);
    rootParams.resize(2);
    ranges1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GraphicSetting::gNumPassConstants, 0);
    rootParams[0].InitAsDescriptorTable(ranges1.size(), ranges1.data());
    ranges2[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GraphicSetting::gNumPerObjectConstants, GraphicSetting::gNumPassConstants);
    rootParams[1].InitAsDescriptorTable(ranges2.size(), ranges2.data());
    
    auto&& staticSamplers = sGetStaticSamplers();
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    desc.Init_1_1(rootParams.size(), rootParams.data(),
        staticSamplers.size(), staticSamplers.data(),
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* blob;
    ID3DBlob* errorBlob;
    if (FAILED((D3D12SerializeVersionedRootSignature(&desc, &blob, &errorBlob))))
    {
        OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
        THROW_EXCEPTION(TEXT("Failed to create root signature"));
    }
    mGlobalRootSignature = D3dContext::instance().createRootSignature(blob);
    blob->Release();
}

void D3dRenderer::registerShaders(std::vector<RHISubShader*> shaders)
{
    for (auto& shader : shaders)
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.VS = {shader->vsBinary().binary(), shader->vsBinary().size() * sizeof(byte) };
        psoDesc.HS = {shader->hsBinary().binary(), shader->hsBinary().size() * sizeof(byte) };
        psoDesc.DS = {shader->dsBinary().binary(), shader->dsBinary().size() * sizeof(byte) };
        psoDesc.GS = {shader->gsBinary().binary(), shader->gsBinary().size() * sizeof(byte) };
        psoDesc.PS = {shader->psBinary().binary(), shader->psBinary().size() * sizeof(byte) };
        const auto& inputElems = RHISubShader::BuildInputElements(*shader);
        D3D12_INPUT_ELEMENT_DESC* d3dInputElems = new D3D12_INPUT_ELEMENT_DESC[inputElems.size()];
        for (int i = 0; i < inputElems.size(); ++i)
        {
            d3dInputElems[i].SemanticName = inputElems[i].mSemanticName.c_str();
            d3dInputElems[i].SemanticIndex = inputElems[i].mSemanticIndex;
            d3dInputElems[i].InputSlot = inputElems[i].mInputSlot;
            d3dInputElems[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            d3dInputElems[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            d3dInputElems[i].InstanceDataStepRate = 0;
        }
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
        inputLayoutDesc.NumElements = inputElems.size();
        inputLayoutDesc.pInputElementDescs = d3dInputElems;
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.pRootSignature = mGlobalRootSignature.Get();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = 0xffffffff;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};

        // rt-ds info
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = static_cast<DXGI_FORMAT>(GraphicSetting::gBackBufferFormat);
        psoDesc.DSVFormat = static_cast<DXGI_FORMAT>(GraphicSetting::gDepthStencilFormat);
        psoDesc.SampleDesc = { GraphicSetting::gSampleCount, GraphicSetting::gSampleQuality };

        mPipelineStates[shader] = D3dContext::instance().createPipelineStateObject(psoDesc);
        delete[] d3dInputElems;
    }
}

void D3dRenderer::appendRenderLists(std::vector<RenderList>&& renderLists)
{
    mPendingRenderLists.reserve(mPendingRenderLists.size() + renderLists.size());
    for (auto& renderList : renderLists)
    {
        mPendingRenderLists.push_back(renderList);
    }
}

ResourceHandle D3dRenderer::allocateDynamicBuffer(uint64_t size)
{
    auto* dynamicBuffer = new DynamicHeap{D3D12RHIFactory::allocDynamicBuffer(size)};
    if (mAvailableResourceAddresses.empty())
    {
        mResources.push_back(dynamicBuffer);
        return { mResources.size() - 1 };
    }
    ResourceHandle resourceHandle = { mAvailableResourceAddresses.top() };
    mResources[resourceHandle.mIndex] = dynamicBuffer;
    return resourceHandle;
}

ResourceHandle D3dRenderer::RHICreateVertexBuffer(uint32_t numVertices, uint16_t vertexSize)
{
    auto* staticBuffer = new StaticHeap{D3D12RHIFactory::allocStaticBuffer(numVertices)};
    if (mAvailableResourceAddresses.empty())
    {
        mResources.push_back(staticBuffer);
        return { mResources.size() - 1 };
    }
    ResourceHandle resourceHandle = { mAvailableResourceAddresses.top() };
    mResources[resourceHandle.mIndex] = staticBuffer;
    return resourceHandle;
}

void D3dRenderer::releaseResource(const ResourceHandle& resourceHandle) const
{
    mReleasingResources[GraphicSetting::gNumBackBuffers].push_back(resourceHandle.mIndex);
}

D3dRenderer::~D3dRenderer() = default;

void D3dRenderer::onPreRender()
{
}

void D3dRenderer::onRender()
{
    // SYNC
    mCopyQueue->Signal(mCopyFence.nativePtr(), ++mCopyFenceValue);
    mMainGraphicQueue->Signal(mGraphicFence.nativePtr(), ++mFrameFenceValue);
    mCopyFence.wait(mCopyFenceValue);
    mGraphicFence.wait(mFrameFenceValue);
    
    D3dCommandListPool::sGetCommandAllocator(CommandListType::DIRECT)->Reset();

    // ------------------------------RenderPath Input---------------------------------
    mGraphicContext.reset(mMainGraphicQueue.Get());
    mGraphicContext.setRenderTargets(mBackBuffers + mCpuWorkingPageIdx, 1);
    mGraphicContext.setDepthStencilBuffer(&mDepthStencilBuffer);
    D3D12_RESOURCE_DESC&& rtDesc = mBackBuffers->nativePtr()->GetDesc();    // TODO:
    Viewport viewport{static_cast<float>(rtDesc.Width), static_cast<float>(rtDesc.Height), 0.0f, 1.0f};
    Rect scissorRect{0, 0, static_cast<LONG>(rtDesc.Width), static_cast<LONG>(rtDesc.Height)};
    D3D12CommandList* pCommandList = D3dCommandListPool::getCommandList(CommandListType::DIRECT);
    
    // ------------------------------RenderPath Start---------------------------------
    ID3D12GraphicsCommandList* nativeCmdList = pCommandList->nativePtr();
    // fill the pass constants part of descriptors in descriptor heap.
    // total pass constants descriptor = constant count per pass * back buffer count
    // they stores in heap like below:
    // | pass0 : constant0, constant1, ... | pass1 : constant0, constant1, ... | ...

    if (mNumDirtyFrames)
    {
        for (int i = 0; i < GraphicSetting::gNumPassConstants; ++i)
        {
            auto* passConstantsBuffer = ;
            memcpy(passConstantsBuffer->mappedPointer(), mPassConstantsData[i].binary(), passConstantsBuffer->size());
        }
        mNumDirtyFrames--;
    }
    
    ID3D12DescriptorHeap* heaps[] = { mCbSrUaDescHeap->nativePtr() };
    nativeCmdList->SetDescriptorHeaps(1, heaps); // do in render
    nativeCmdList->SetGraphicsRootSignature(mGlobalRootSignature.Get()); // do in dc
    nativeCmdList->SetGraphicsRootDescriptorTable(0, mCbSrUaDescHeap->gpuHandle(sCalcPassCbvStartIdx(mCpuWorkingPageIdx)));
    
    // set render target
    D3D12_CPU_DESCRIPTOR_HANDLE hRTV = mRtDescHeap->cpuHandle(mCpuWorkingPageIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE hDSV = mDsDescHeap->cpuHandle(0);

    pCommandList->transition(mBackBuffers[mCpuWorkingPageIdx], ResourceState::RENDER_TARGET);
    
    // ----------------------------------Pass Start-----------------------------------
    for (const auto& renderList : mPendingRenderLists)
    {
        TransformConstants transform{};
        transform.mView = renderList.mView;
        transform.mProjection = renderList.mProj;
        // update buffers
        nativeCmdList->OMSetRenderTargets(1, &hRTV, false, &hDSV);
        nativeCmdList->ClearRenderTargetView(hRTV, DirectX::Colors::LightSteelBlue, 0, nullptr);
    
        nativeCmdList->ClearDepthStencilView(hDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        // Rasterize
        nativeCmdList->RSSetViewports(1, reinterpret_cast<const D3D12_VIEWPORT*>(&viewport));
        nativeCmdList->RSSetScissorRects(1, reinterpret_cast<const D3D12_RECT*>(&scissorRect));
        // ------------------------------Draw Call Begin----------------------------------
        for (uint64_t i = 0; i < renderList.mRenderItems.size(); ++i)
        {
            nativeCmdList->SetGraphicsRootDescriptorTable(1, mCbSrUaDescHeap->gpuHandle(sCalcObjectCbvOffset(mCpuWorkingPageIdx)));
            uint64_t objectConstantsStart = GraphicSetting::gNumPassConstants + GraphicSetting::gNumPerObjectConstants * i;
            const auto& renderItem = renderList.mRenderItems[i];
            const auto& materialConstants = renderItem.mMaterial->mConstant;
            // copy all constants data to dynamic buffers that bound to the registers through descriptors heap.
            // register 1 is bound to per-object transform data
            auto& transformBuffer = mRenderData[mCpuWorkingPageIdx].mCBuffers[objectConstantsStart];
            transform.mModel = renderItem.mModel;
            memcpy(transformBuffer->mappedPointer(), &transform, sizeof(TransformConstants));
            // copy material data
            for (auto& constant : materialConstants)
            {
                auto& constantBuffer = mRenderData[mCpuWorkingPageIdx].mCBuffers[objectConstantsStart + constant.first]; 
                memcpy(constantBuffer->mappedPointer(), constant.second.data(), constant.second.size());    // TODO: grow dynamic buffer if needed
            }
            const auto& meshData = renderItem.mMeshData;
            uint32_t vertexSize = renderItem.mMaterial->shader->vertexSize();
            D3dResource* vertexBuffer = mResources[meshData.mVertexBuffer.mIndex];
            D3dResource* indexBuffer = mResources[meshData.mIndexBuffer.mIndex];
            D3D12_VERTEX_BUFFER_VIEW vBufferDesc = {
                vertexBuffer->nativePtr()->GetGPUVirtualAddress(),
                (meshData.mVertexCount * vertexSize), vertexSize
            };
            D3D12_INDEX_BUFFER_VIEW iBufferDesc = {
                indexBuffer->nativePtr()->GetGPUVirtualAddress(),
                meshData.mIndexCount * static_cast<uint32_t>(sizeof(uint32_t)), DXGI_FORMAT_R32_UINT
            };
            nativeCmdList->SetPipelineState(mPipelineStates[renderItem.mMaterial->shader]);
            // Input Assemble
            nativeCmdList->IASetVertexBuffers(0, 1, &vBufferDesc);
            nativeCmdList->IASetIndexBuffer(&iBufferDesc);
            nativeCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            // Draw Call
            for (const auto& subMesh : renderItem.mMeshData.mSubMeshes)
            {
                nativeCmdList->DrawIndexedInstanced(subMesh.mIndexNum, 1, subMesh.mStartIndex, subMesh.mBaseVertex, 0);
            }
        }
        // -------------------------------Draw Call End-----------------------------------
    }
    pCommandList->transition(mBackBuffers[mCpuWorkingPageIdx], ResourceState::PRESENT);
    pCommandList->close();
    mGraphicContext.executeCommandList(pCommandList);
    ThrowIfFailed(mSwapChain->Present(1, 0));

    // -------------------------------Release Resources-------------------------------
    mPendingRenderLists.clear();
    auto& releasingResources = mReleasingResources[mCpuWorkingPageIdx]; 
    releasingResources.clear();
    releasingResources.swap(mReleasingResources[GraphicSetting::gNumBackBuffers]);
    for (auto index : releasingResources)
    {
        mResources[index]->Release();
        delete mResources[index];
        mResources[index] = nullptr;
        mAvailableResourceAddresses.push(index);
    }
    
    mCpuWorkingPageIdx = (mCpuWorkingPageIdx + 1) % GraphicSetting::gNumBackBuffers;
}

void D3dRenderer::render()
{
    onPreRender();
    onRender();
}

void D3dRenderer::release()
{
    // for (auto& pair : mRenderPaths)
    // {
    //     pair.first->Release();
    //     for (int i = 0; i < mGraphicSettings.gNumBackBuffers; ++i)
    //     {
    //         pair.second[i]->Release();
    //         delete pair.second[i];
    //     }
    //     delete pair.second;
    // }
    // mRenderPaths.Release();
    delete[] mReleasingResources;
    mGraphicFence.wait(mFrameFenceValue - GraphicSetting::gNumBackBuffers);
    mGraphicFence.Release();
    mSwapChain.Reset();
    for (int i = 0; i < GraphicSetting::gNumBackBuffers; ++i)
    {
        mBackBuffers[i].Release();
    }
    mDepthStencilBuffer.Release();
    mMainGraphicQueue.Reset();
    mGlobalRootSignature->Release();
}

D3dRenderer::D3dRenderer() = default;
#endif