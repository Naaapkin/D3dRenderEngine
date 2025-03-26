#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Resource/D3dAllocator.h"
#include "Engine/render/PC/Resource/RenderItem.h"
#ifdef WIN32
#include "D3dRenderer.h"
#include "Engine/common/Exception.h"
#include "Engine/render/PC/Core/D3dContext.h"
#include "Engine/render/PC/Resource/Shader.h"
#include "Engine/render/PC/Resource/RenderTexture.h"

Renderer* Renderer::sRenderer()
{
    return D3dRenderer::sD3dRenderer();
}

void Renderer::sInitialize()
{
    D3dRenderer& renderer = dynamic_cast<D3dRenderer&>(*sRenderer());
    if (renderer.mD3dContext) return;
    HWND hwndActive = GetActiveWindow();
    if (!hwndActive)
    {
        // TODO: log fail info
        return;
    }
    renderer.initializeImpl(hwndActive);
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

uint8_t D3dRenderer::sNumCpuWorkPages()
{
    return sD3dRenderer()->mGraphicSettings.mNumBackBuffers;
}

const GraphicSetting& D3dRenderer::sGraphicSettings()
{
    return sD3dRenderer()->mGraphicSettings;
}

bool D3dRenderer::sIsRenderingThread()
{
    return std::this_thread::get_id() == sD3dRenderer()->mRenderingThreadId;
}

std::vector<D3D12_STATIC_SAMPLER_DESC> D3dRenderer::sGetStaticSamplers() {
    return {
        // 点采样器（用于像素精确采样）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
             0.0f,
            D3D12_FLOAT32_MAX,
            0,  // 绑定到 s0
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 线性采样器（用于平滑插值）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            0.0f,
            D3D12_FLOAT32_MAX,
            1,  // 绑定到 s1
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 线性采样器（用于平滑插值）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            0.0f,
            D3D12_FLOAT32_MAX,
            2,  // 绑定到 s2
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 各向异性采样器（用于高质量纹理）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            16,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            0.0f,
            D3D12_FLOAT32_MAX,
            3,  // 绑定到 s3
            0,
            D3D12_SHADER_VISIBILITY_ALL
        }
    };
}

uint64_t D3dRenderer::sCalcPassCbvStartIdx(const GraphicSetting& graphicSettings, uint8_t cpuWorkingPageIdx)
{
    return cpuWorkingPageIdx * graphicSettings.mNumPassConstants;
}

uint64_t D3dRenderer::sCalcObjectCbvStartIdx(const GraphicSetting& graphicSettings, uint8_t cpuWorkingPageIdx)
{
    return cpuWorkingPageIdx * graphicSettings.mMaxRenderItemsPerFrame + graphicSettings.mNumPassConstants * graphicSettings.mNumBackBuffers;
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
    ranges1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, mGraphicSettings.mNumPassConstants, 0);
    rootParams[0].InitAsDescriptorTable(ranges1.size(), ranges1.data());
    ranges2[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, mGraphicSettings.mNumPerObjectConstants, mGraphicSettings.mNumPassConstants);
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
    mGlobalRootSignature = mD3dContext->createRootSignature(blob);
    blob->Release();
}

D3dRenderer::~D3dRenderer() = default;

void D3dRenderer::initializeImpl(HWND hWindow)
{
    mRenderingThreadId = std::this_thread::get_id();
    
    mD3dContext.reset(new D3dContext(hWindow));
    mD3dContext->initialize();
    D3dAllocator allocator{mD3dContext.get()};

    mMainGraphicQueue = mD3dContext->createCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // create swapchain and backbuffers
    mSwapChain.Attach(
        mD3dContext->createSwapChainForHwnd(mMainGraphicQueue.Get(), mGraphicSettings.mBackBufferFormat,
                                            mGraphicSettings.mNumBackBuffers)
            );
    DXGI_SWAP_CHAIN_DESC1 desc;
    mSwapChain->GetDesc1(&desc);

    createRootSignature();

    mRtDescHeap.reset(mD3dContext->createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        mGraphicSettings.mMaxNumRenderTarget));
    mDsDescHeap.reset(mD3dContext->createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        mGraphicSettings.mMaxNumRenderTarget));
    mCbSrUaDescHeap.reset(mD3dContext->createDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        mGraphicSettings.mNumBackBuffers * (mGraphicSettings.mNumPassConstants + mGraphicSettings.mMaxRenderItemsPerFrame * mGraphicSettings.mNumPerObjectConstants)));

    // TODO: 
    D3D12_CPU_DESCRIPTOR_HANDLE descHandle = mRtDescHeap->nativePtr()->GetCPUDescriptorHandleForHeapStart();
    mBackBuffers = new RenderTexture2D[mGraphicSettings.mNumBackBuffers];
    for (int i = 0; i < mGraphicSettings.mNumBackBuffers; ++i)
    {
        ID3D12Resource* backBuffers;
        ThrowIfFailed(
            mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffers))
        );
        mD3dContext->deviceHandle()->CreateRenderTargetView(backBuffers, nullptr, descHandle);
        descHandle.ptr += mRtDescHeap->descriptorSize();
        mBackBuffers[i] = std::move( RenderTexture2D{ D3dResource{ backBuffers, 1, static_cast<ResourceState>(D3D12_RESOURCE_STATE_COMMON) } });
    }
    mCpuWorkingPageIdx = 0;
    // create depth stencil buffer
    mDepthStencilBuffer = std::move(
        *allocator.allocDepthStencilResource(desc.Width, desc.Height, mGraphicSettings.mDepthStencilFormat));

    mFrameFence = std::move(mD3dContext->createFence(0));
    mFrameFenceValue = 0;

    D3dCommandListPool::initialize(*mD3dContext);
}

void D3dRenderer::registerShaders(const std::vector<Shader*>& shaders)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    for (Shader* shader : shaders)
    {
        ID3DBlob* binary = const_cast<ID3DBlob*>(shader->vsBinary());
        if (binary) psoDesc.VS = {binary->GetBufferPointer(), binary->GetBufferSize() };
        binary = const_cast<ID3DBlob*>(shader->hsBinary());
        if (binary) psoDesc.HS = { binary->GetBufferPointer(), binary->GetBufferSize() };
        binary = const_cast<ID3DBlob*>(shader->dsBinary());
        if (binary) psoDesc.DS = { binary->GetBufferPointer(), binary->GetBufferSize() };
        binary = const_cast<ID3DBlob*>(shader->gsBinary());
        if (binary) psoDesc.GS = { binary->GetBufferPointer(), binary->GetBufferSize() };
        binary = const_cast<ID3DBlob*>(shader->psBinary());
        if (binary) psoDesc.PS = { binary->GetBufferPointer(), binary->GetBufferSize() };
        
        // D3D12_INPUT_ELEMENT_DESC inputDesc[] = {
        //     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        //     {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
        //     {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
        //     {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
        //     {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA}
        // };
        // D3D12_INPUT_LAYOUT_DESC layoutDesc = { inputDesc, 5 };
        D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = shader->inputLayout();
        psoDesc.InputLayout = inputLayoutDesc;
        psoDesc.pRootSignature = mGlobalRootSignature;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.SampleMask = 0xffffffff;
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC{D3D12_DEFAULT};

        // rt-ds info
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = static_cast<DXGI_FORMAT>(mGraphicSettings.mBackBufferFormat);
        psoDesc.DSVFormat = static_cast<DXGI_FORMAT>(mGraphicSettings.mDepthStencilFormat);
        psoDesc.SampleDesc = mGraphicSettings.mSampleDesc;

        mPipelineStates[shader] = mD3dContext->createPipelineStateObject(psoDesc);
    }
}

ResourceHandle D3dRenderer::allocateDynamicBuffer(uint64_t size)
{
    return {};
}

ResourceHandle D3dRenderer::allocateDefaultBuffer(uint64_t size)
{
    return {};
}

void D3dRenderer::updateDynamicBuffer(ResourceHandle buffer, const void* data, uint64_t size)
{
}

void D3dRenderer::updateDefaultBuffer(ResourceHandle buffer, const void* data, uint64_t size)
{
}

void D3dRenderer::render()
{
    mFrameFence.wait(mFrameFenceValue - mCpuWorkingPageIdx);
    D3dCommandListPool::sGetCommandAllocator()->Reset();
    
    // ------------------------------RenderPath Input---------------------------------
    RenderContext renderContext{};
    renderContext.setCommandQueue(mMainGraphicQueue.Get());
    renderContext.setRenderTargets(mBackBuffers + mCpuWorkingPageIdx, 1);
    renderContext.setDepthStencilBuffer(&mDepthStencilBuffer);
    D3D12_RESOURCE_DESC&& rtDesc = mBackBuffers->nativePtr()->GetDesc();    // TODO:
    Viewport viewport{static_cast<float>(rtDesc.Width), static_cast<float>(rtDesc.Height), 0.0f, 1.0f};
    Rect scissorRect{0, 0, static_cast<LONG>(rtDesc.Width), static_cast<LONG>(rtDesc.Height)};
    D3dCommandList* commandList = D3dCommandListPool::getCommandList();
    
    // ------------------------------RenderPath Start---------------------------------
    ID3D12GraphicsCommandList* nativeCmdList = commandList->nativePtr();
    ID3D12DescriptorHeap* heaps[] = { mCbSrUaDescHeap->nativePtr() };
    nativeCmdList->SetDescriptorHeaps(1, heaps); // do in render
    nativeCmdList->SetGraphicsRootSignature(mGlobalRootSignature); // do in dc
    nativeCmdList->SetGraphicsRootDescriptorTable(0, mCbSrUaDescHeap->gpuHandle(sCalcPassCbvStartIdx(mGraphicSettings, mCpuWorkingPageIdx)));
    nativeCmdList->SetGraphicsRootDescriptorTable(1, mCbSrUaDescHeap->gpuHandle(sCalcObjectCbvStartIdx(mGraphicSettings, mCpuWorkingPageIdx)));
    
    // set render target
    D3D12_CPU_DESCRIPTOR_HANDLE&& hRTV = mRtDescHeap->cpuHandle(mCpuWorkingPageIdx);
    D3D12_CPU_DESCRIPTOR_HANDLE&& hDSV = mDsDescHeap->cpuHandle(0);

    // ----------------------------------Pass Start-----------------------------------
    for (const auto& renderList : mPendingRenderLists)
    {
        nativeCmdList->OMSetRenderTargets(1, &hRTV, false, &hDSV);
        nativeCmdList->ClearRenderTargetView(hRTV, DirectX::Colors::LightSteelBlue, 0, nullptr);
    
        nativeCmdList->ClearDepthStencilView(hDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        // Rasterize
        nativeCmdList->RSSetViewports(1, reinterpret_cast<const D3D12_VIEWPORT*>(&viewport));
        nativeCmdList->RSSetScissorRects(1, reinterpret_cast<const D3D12_RECT*>(&scissorRect));
        // ------------------------------Draw Call Begin----------------------------------
        for (const auto& renderItem : renderList.mRenderItems)
        {
            const auto& meshData = renderItem.mMeshData;
            uint32_t vertexSize = renderItem.mMaterial.shader->vertexSize();
            D3dResource* vertexBuffer = mResources[meshData.mVertexBuffer.mIdx * mGraphicSettings.mNumBackBuffers + mCpuWorkingPageIdx];
            D3dResource* indexBuffer = mResources[meshData.mIndexBuffer.mIdx * mGraphicSettings.mNumBackBuffers + mCpuWorkingPageIdx];
            D3D12_VERTEX_BUFFER_VIEW vBufferDesc = {
                vertexBuffer->nativePtr()->GetGPUVirtualAddress(),
                (meshData.mVertexCount * vertexSize), vertexSize
            };
            D3D12_INDEX_BUFFER_VIEW iBufferDesc = {
                indexBuffer->nativePtr()->GetGPUVirtualAddress(),
                meshData.mIndexCount * static_cast<uint32_t>(sizeof(uint32_t)), DXGI_FORMAT_R32_UINT
            };
            // Input Assemble
            nativeCmdList->IASetVertexBuffers(0, 1, &vBufferDesc);
            nativeCmdList->IASetIndexBuffer(&iBufferDesc);
            nativeCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            nativeCmdList->SetPipelineState(mPipelineStates[renderItem.mMaterial.shader]);
            // Draw Call
            for (const auto& subMesh : renderItem.mMeshData.mSubMeshes)
            {
                nativeCmdList->DrawIndexedInstanced(subMesh.mIndexNum, 1, subMesh.mStartIndex, subMesh.mBaseVertex, 0);
            }
        }
        // -------------------------------Draw Call End-----------------------------------
    }
    
    // for (auto pair : mRenderPasses)
    // {
    //     pair.first->render(renderContext, commandList);
    // }
    mCpuWorkingPageIdx = (mCpuWorkingPageIdx + 1) % mGraphicSettings.mNumBackBuffers;
    mMainGraphicQueue->Signal(mFrameFence.nativePtr(), (++mFrameFenceValue) - mCpuWorkingPageIdx);
}

void D3dRenderer::release()
{
    // for (auto& pair : mRenderPaths)
    // {
    //     pair.first->release();
    //     for (int i = 0; i < mGraphicSettings.mNumBackBuffers; ++i)
    //     {
    //         pair.second[i]->release();
    //         delete pair.second[i];
    //     }
    //     delete pair.second;
    // }
    // mRenderPaths.clear();
    mFrameFence.wait(mFrameFenceValue - mCpuWorkingPageIdx);
    mFrameFence.release();
    mSwapChain.Reset();
    for (int i = 0; i < mGraphicSettings.mNumBackBuffers; ++i)
    {
        mBackBuffers[i].release();
    }
    mDepthStencilBuffer.release();
    mMainGraphicQueue.Reset();
    mGlobalRootSignature->Release();
}

D3dRenderer::D3dRenderer() = default;
#endif