#ifdef WIN32
#include "D3D12RHI.h"

#include "Native/D3D12CommandObjectPool.h"
#include "Native/D3D12PipelineStateManager.h"
#include "Private/D3D12GraphicsContext.h"
#include "Private/D3D12RenderTarget.h"
#include "Resource/D3D12Fence.h"
#include "Resource/D3D12Resources.h"
#include "Private/D3D12SwapChain.h"

void D3D12RHI::Initialize(const RHIConfiguration& configuration)
{
    static constexpr uint8_t COMMAND_LIST_CAPACITY = 32;
    static constexpr uint8_t COMMAND_ALLOCATOR_CAPACITY = 8;

    RHI::Initialize(configuration);
    mDevice = D3D12Device::CreateD3D12Device();

    mPipelineStateManager.reset(new D3D12PipelineStateManager{});
    mRootSignatureManager.reset(new D3D12RootSignatureManager{});
    mCommandObjectPool.reset(new D3D12CommandObjectPool{});
    mConstantBufferAllocator.reset(new D3D12RingBufferAllocator{});
    mStagingBufferAllocator.reset(new D3D12RingBufferAllocator{});

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    mDirectQueue = mDevice->CreateCommandQueue(desc);
    desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    mCopyQueue = mDevice->CreateCommandQueue(desc);
    desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    mComputeQueue = mDevice->CreateCommandQueue(desc);

    mPipelineStateManager->Initialize(mDevice.get());
    mCommandObjectPool->Initialize(mDevice.get(), COMMAND_LIST_CAPACITY, COMMAND_ALLOCATOR_CAPACITY);
    mRootSignatureManager->Initialize(mDevice.get());
    mConstantBufferAllocator->Initialize(mDevice.get(), 12 * 1024 * 1024); // TODO:
    mStagingBufferAllocator->Initialize(mDevice.get(), 64 * 1024 * 1024);   // 64MB

    mOnlineCBVSRVUAVAllocator.reset(new RingDescriptorAllocator{});
    mRTVAllocator.reset(new BlockDescriptorAllocator{});
    mDSVAllocator.reset(new BlockDescriptorAllocator{});
    
    // -------------------Using Global Signature Temporary---------------------
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
    std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1[]> rootDescriptorRanges;
    const RootSignatureLayout& layout = configuration.mRootSignatureLayout;
    BuildGlobalRootSignature(rootDescriptorRanges, rootParameters, samplers);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC::Init_1_1(rootSignatureDesc, rootParameters.size(), rootParameters.data(),
                                                    samplers.size(), samplers.data(),
                                                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    mRootSignatureManager->AppendSignature("UniversalRootSignature", rootSignatureDesc, layout);
    // ------------------------------------------------------------------------
    
    // ------------------------D3D12DescriptorManager--------------------------
    mOnlineCBVSRVUAVAllocator->Initialize(mDevice.get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8192, true);
    mRTVAllocator->Initialize(mDevice.get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, configuration.mMaxNumRenderTarget, false);
    mDSVAllocator->Initialize(mDevice.get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, configuration.mMaxNumDepthStencil, false);
    // ------------------------------------------------------------------------
    
    mShaderBinaryAllocator.Initialize();

    mCommandContext.Initialize1(mDevice.get(), mOnlineCBVSRVUAVAllocator.get(), mPipelineStateManager.get(), mRootSignatureManager.get());
    //mCommandContext.Initialize2(mDirectQueue.Get(), mCopyQueue.Get(), mComputeQueue.Get());
    // Singleton<D3D12BuddyCBufferAllocator>::GetInstance().Initialize1(mDevice.get(), 256 * 1024 * 1024); // 256 MB
}

std::unique_ptr<RHIShader> D3D12RHI::RHICompileShader(const Blob& binary, ShaderType activeTypes, const std::string* path)
{
    std::vector<ShaderInput> inputs;
    std::unordered_set<ShaderProp> props;
    RHIShader* pShader = new RHIShader();
    // pShader->BindShaderResources(Singleton<D3D12RootSignatureManager>::GetInstance().GetByIndex(0));   // TODO:
    UComPtr<ID3DBlob> vs;
    UComPtr<ID3DBlob> hs;
    UComPtr<ID3DBlob> ds;
    UComPtr<ID3DBlob> gs;
    UComPtr<ID3DBlob> ps;
    const char* pathCStr = path ? path->c_str() : nullptr;
    if (activeTypes & ShaderType::VERTEX)
    {
        vs = ::D3D12Compile(binary, "VsMain", "vs_5_0", pathCStr);
        UComPtr<ID3D12ShaderReflection> pReflection;
        ThrowIfFailed(D3DReflect(vs->GetBufferPointer(), vs->GetBufferSize(), IID_PPV_ARGS(pReflection.GetAddressOf())));
        GetShaderProperties(ShaderType::VERTEX, pReflection.Get(), props);
        GetShaderInputElements(pReflection.Get(), inputs);
    }
    if (activeTypes & ShaderType::HULL)
    {
        hs = ::D3D12Compile(binary, "HsMain", "hs_5_0", pathCStr);
        UComPtr<ID3D12ShaderReflection> pReflection;
        ThrowIfFailed(D3DReflect(hs->GetBufferPointer(), hs->GetBufferSize(), IID_PPV_ARGS(pReflection.GetAddressOf())));
        GetShaderProperties(ShaderType::HULL, pReflection.Get(), props);
    }
    if (activeTypes & ShaderType::DOMAIN)
    {
        ds = ::D3D12Compile(binary, "DsMain", "ds_5_0", pathCStr);
        UComPtr<ID3D12ShaderReflection> pReflection;
        ThrowIfFailed(D3DReflect(ds->GetBufferPointer(), ds->GetBufferSize(), IID_PPV_ARGS(pReflection.GetAddressOf())));
        GetShaderProperties(ShaderType::DOMAIN, pReflection.Get(), props);
    }
    if (activeTypes & ShaderType::GEOMETRY)
    {
        gs = ::D3D12Compile(binary, "GsMain", "gs_5_0", pathCStr);
        UComPtr<ID3D12ShaderReflection> pReflection;
        ThrowIfFailed(D3DReflect(gs->GetBufferPointer(), gs->GetBufferSize(), IID_PPV_ARGS(pReflection.GetAddressOf())));
        GetShaderProperties(ShaderType::GEOMETRY, pReflection.Get(), props);
    }
    if (activeTypes & ShaderType::PIXEL)
    {
        ps = ::D3D12Compile(binary, "PsMain", "ps_5_0", pathCStr);
        UComPtr<ID3D12ShaderReflection> pReflection;
        ThrowIfFailed(D3DReflect(ps->GetBufferPointer(), ps->GetBufferSize(), IID_PPV_ARGS(pReflection.GetAddressOf())));
        GetShaderProperties(ShaderType::PIXEL, pReflection.Get(), props);
    }
    pShader->SetShaders(
        vs ? Blob{ vs->GetBufferPointer(), vs->GetBufferSize() } : Blob{ nullptr, 0 },
        hs ? Blob{ hs->GetBufferPointer(), hs->GetBufferSize() } : Blob{ nullptr, 0 },
        ds ? Blob{ ds->GetBufferPointer(), ds->GetBufferSize() } : Blob{ nullptr, 0 },
        gs ? Blob{ gs->GetBufferPointer(), gs->GetBufferSize() } : Blob{ nullptr, 0 },
        ps ? Blob{ ps->GetBufferPointer(), ps->GetBufferSize() } : Blob{ nullptr, 0 });
    pShader->SetShaderInputs(inputs);
    pShader->SetShaderProperties({props.begin(), props.end()});
    return std::unique_ptr<RHIShader>(pShader);
}

//std::unique_ptr<RHINativeBuffer> D3D12RHI::RHIAllocBuffer(uint64_t size)
//{
//    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
//    D3D12_RESOURCE_DESC d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(size);
//    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, nullptr,
//                                                                        D3D12_RESOURCE_STATE_COMMON);
//    return std::make_unique<D3D12Buffer>(pResource.Detach(), RHIBufferDesc{size, ResourceType::STATIC});
//}

std::unique_ptr<RHIStagingBuffer> D3D12RHI::RHIAllocStagingBuffer(uint64_t size)
{
    RHINativeBuffer* pCBuffer = new D3D12StagingBuffer(mStagingBufferAllocator->Allocate(size));
    return std::make_unique<RHIStagingBuffer>(std::unique_ptr<RHINativeBuffer>(pCBuffer));
}

std::unique_ptr<RHIStagingBuffer> D3D12RHI::RHIAllocStagingTexture(uint64_t size)
{
    RHINativeBuffer* pCBuffer = new D3D12StagingBuffer(mStagingBufferAllocator->AllocateAligned(size, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT));
    return std::make_unique<RHIStagingBuffer>(std::unique_ptr<RHINativeBuffer>(pCBuffer));
}

std::unique_ptr<RHIConstantBuffer> D3D12RHI::RHIAllocConstantBuffer(uint64_t size)
 {
     RHINativeBuffer* pCBuffer = new D3D12ConstantBuffer(
         mConstantBufferAllocator->Allocate(
	         ::AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(size)));
     return std::make_unique<RHIConstantBuffer>(std::unique_ptr<RHINativeBuffer>(pCBuffer));
 }

std::unique_ptr<RHINativeTexture> D3D12RHI::RHIAllocTexture(RHITextureDesc desc)
{
    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    desc.mMipLevels = desc.mMipLevels ? desc.mMipLevels : GetMipLevelCount(desc.mWidth, desc.mHeight, desc.mDepth);
    D3D12_RESOURCE_DESC d3d12Desc;
    switch (desc.mDimension)
    {
    default:
    case TextureDimension::TEXTURE2D:
        d3d12Desc = CD3DX12_RESOURCE_DESC::Tex2D(::ConvertToDXGIFormat(desc.mFormat), desc.mWidth, desc.mHeight, desc.mDepth,
                                     desc.mMipLevels,
                                     desc.mSampleCount, desc.mSampleQuality);
        break;
    case TextureDimension::TEXTURE3D:
        d3d12Desc = CD3DX12_RESOURCE_DESC::Tex3D(::ConvertToDXGIFormat(desc.mFormat), desc.mWidth, desc.mHeight, desc.mDepth,
                                 desc.mMipLevels);
        break;
    }
    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, nullptr,
                                                                        D3D12_RESOURCE_STATE_COMMON);
    D3D12Texture* pTexture = new D3D12Texture(std::move(pResource), desc);
    ResourceStateTracker::AppendResource(pTexture->GetD3D12Resource(), ResourceState::COMMON);
    return std::unique_ptr<D3D12Texture>(pTexture);
}

std::unique_ptr<RHIDepthStencil> D3D12RHI::RHIAllocDepthStencil(RHITextureDesc desc)
{
    ASSERT(desc.mDimension == TextureDimension::TEXTURE2D, TEXT("depth stencil buffer should be 2-dimension texture."));
    desc.mMipLevels = desc.mMipLevels ? desc.mMipLevels : GetMipLevelCount(desc.mWidth, desc.mHeight, desc.mDepth);
    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC d3d12Desc = CD3DX12_RESOURCE_DESC::Tex2D(::ConvertToDXGIFormat(desc.mFormat), desc.mWidth, desc.mHeight, 1,
                                     desc.mMipLevels,
                                     desc.mSampleCount, desc.mSampleQuality);
    d3d12Desc.Flags = d3d12Desc.Flags | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE{d3d12Desc.Format, .0, 0};
    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, &clearValue,
                                                                        D3D12_RESOURCE_STATE_COMMON);
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDSVAllocator->Allocate()[0].mCPUHandle;
    mDevice->CreateDepthStencilView(pResource.Get(), dsvHandle);
    D3D12DepthStencil* pTexture = new D3D12DepthStencil(desc, pResource.Detach(), dsvHandle);
    ResourceStateTracker::AppendResource(pTexture->GetD3D12Resource(), ResourceState::COMMON);
    return std::unique_ptr<D3D12DepthStencil>(pTexture);
}

std::unique_ptr<RHIRenderTarget> D3D12RHI::RHIAllocRenderTarget(RHITextureDesc desc)
{
    desc.mMipLevels = desc.mMipLevels ? desc.mMipLevels : GetMipLevelCount(desc.mWidth, desc.mHeight, desc.mDepth);
    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC d3d12Desc = CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(desc.mFormat), desc.mWidth, desc.mHeight, 1,
                                     desc.mMipLevels,
                                     desc.mSampleCount, desc.mSampleQuality);
    d3d12Desc.Flags = d3d12Desc.Flags | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, nullptr,
                                                                        D3D12_RESOURCE_STATE_COMMON);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVAllocator->Allocate()[0].mCPUHandle;
    mDevice->CreateRenderTargetView(pResource.Get(), rtvHandle);
    D3D12RenderTarget* pTexture = new D3D12RenderTarget(desc, pResource.Detach(), rtvHandle);
    ResourceStateTracker::AppendResource(pTexture->GetD3D12Resource(), ResourceState::PRESENT);
    return std::unique_ptr<D3D12RenderTarget>(pTexture);
}

std::unique_ptr<RHIVertexBuffer> D3D12RHI::RHIAllocVertexBuffer(uint64_t vertexSize,
                                                                 uint64_t numVertices)
{
    uint64_t size = vertexSize * numVertices;
    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, nullptr,
        D3D12_RESOURCE_STATE_COMMON);
    D3D12VertexBuffer* pVertexBuffer = new D3D12VertexBuffer(std::move(pResource), RHIBufferDesc{ size, ResourceType::STATIC }, vertexSize, numVertices);
    ResourceStateTracker::AppendResource(pVertexBuffer->GetD3D12Resource(), ResourceState::COMMON);
    return std::make_unique<RHIVertexBuffer>(std::unique_ptr<RHINativeBuffer>(pVertexBuffer));
}

std::unique_ptr<RHIIndexBuffer> D3D12RHI::RHIAllocIndexBuffer(uint64_t numIndices, Format indexFormat)
{
    uint64_t size;
    if (indexFormat == Format::R32_UINT)
    {
        size = sizeof(uint32_t) * numIndices;
    }
    else if (indexFormat == Format::R16_UINT)
    {
        size = sizeof(uint16_t) * numIndices;
    }
    else
    {
        THROW_EXCEPTION(TEXT("invalid index format!"));
    }
    D3D12_HEAP_PROPERTIES prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC d3d12Desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    UComPtr<ID3D12Resource> pResource = mDevice->CreateCommitedResource(prop, D3D12_HEAP_FLAG_NONE, d3d12Desc, nullptr,
        D3D12_RESOURCE_STATE_COMMON);
    D3D12IndexBuffer* pIndexBuffer = new D3D12IndexBuffer(std::move(pResource), RHIBufferDesc{ size, ResourceType::STATIC }, numIndices, indexFormat);
    ResourceStateTracker::AppendResource(pIndexBuffer->GetD3D12Resource(), ResourceState::COMMON);
    return std::make_unique<RHIIndexBuffer>(std::unique_ptr<RHINativeBuffer>(pIndexBuffer));
}

std::unique_ptr<RHIFence> D3D12RHI::RHICreateFence()
{
    return std::make_unique<D3D12Fence>(mDevice->CreateFence().Detach());
}

std::unique_ptr<RHISwapChain> D3D12RHI::RHICreateSwapChain(const RHISwapChainDesc& desc)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; swapChainDesc.Height = 0;  // use 0 to fetch the current window Size
    swapChainDesc.Format = ConvertToDXGIFormat(desc.mFormat);
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc = DXGI_SAMPLE_DESC{desc.mMSAA, static_cast<uint32_t>(desc.mMSAA > 1 ? 1 : 0)};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = desc.mNumBackBuffers;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;
    
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainDescFullscreen;
    swapChainDescFullscreen.Scaling = DXGI_MODE_SCALING_CENTERED;
    swapChainDescFullscreen.Windowed = !desc.mIsFullScreen;
    swapChainDescFullscreen.RefreshRate = {desc.mTargetFramesPerSec, 1 };
    swapChainDescFullscreen.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    
    UComPtr<IDXGISwapChain1> pSwapChain = mDevice->CreateSwapChain(mDirectQueue.Get(), swapChainDesc, &swapChainDescFullscreen);
    
    std::unique_ptr<RHIRenderTarget*[]> renderTargets;
    renderTargets.reset(new RHIRenderTarget*[desc.mNumBackBuffers]);
    RHITextureDesc backBufferDesc = {desc.mFormat, TextureDimension::TEXTURE2D, 1, 1, 1, 1, desc.mMSAA, static_cast<uint8_t>(desc.mMSAA > 1 ? 1 : 0)};
    for (int i = 0; i < desc.mNumBackBuffers; ++i)
    {
        ID3D12Resource* pD3D12BackBuffer;
        pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pD3D12BackBuffer));
        D3D12_RESOURCE_DESC d3d12BackBufferDesc = pD3D12BackBuffer->GetDesc();
        backBufferDesc.mWidth = d3d12BackBufferDesc.Width;
        backBufferDesc.mHeight = d3d12BackBufferDesc.Height;
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRTVAllocator->Allocate()[0].mCPUHandle;
        mDevice->CreateRenderTargetView(pD3D12BackBuffer, rtvHandle);
        D3D12RenderTarget* pRenderTarget = new D3D12RenderTarget(backBufferDesc, pD3D12BackBuffer, rtvHandle);
        ResourceStateTracker::AppendResource(pRenderTarget->GetD3D12Resource(), ResourceState::PRESENT);
        renderTargets[i] = pRenderTarget;
    }

    return std::make_unique<D3D12SwapChain>(std::move(pSwapChain), std::move(renderTargets), desc.mNumBackBuffers);
}

void D3D12RHI::UpdateStagingBuffer(RHIStagingBuffer* pBuffer, const void* pData, uint64_t offset,
                                   uint64_t size)
{
    static_cast<D3D12StagingBuffer*>(pBuffer->GetBuffer())->Update(pData, offset, size);
}

void D3D12RHI::UpdateConstantBuffer(RHIConstantBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size)
{
    static_cast<D3D12ConstantBuffer*>(pBuffer->GetBuffer())->Update(pData, offset, size);
}

void D3D12RHI::CreateGraphicsContext(RHIGraphicsContext** ppContext)
{
    D3D12GraphicsContext* pD3D12GraphicsContext = new D3D12GraphicsContext{};
    pD3D12GraphicsContext->Initialize(&mCommandContext, mCommandObjectPool->ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT), mStagingBufferAllocator->GetD3D12Resource());
    *ppContext = pD3D12GraphicsContext;
}

void D3D12RHI::CreateCopyContext(RHICopyContext** ppContext)
{
    D3D12CopyContext* pD3D12CopyContext = new D3D12CopyContext{};
    //D3D12CopyCommandContext* pCommandContext = new D3D12CopyCommandContext();
    //pCommandContext->Initialize1();
    pD3D12CopyContext->Initialize(mStagingBufferAllocator->GetD3D12Resource(),
                                  mCommandObjectPool->ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY));
    *ppContext = pD3D12CopyContext;
}

void D3D12RHI::CreateComputeContext(RHIComputeContext** ppContext)
{
}

void D3D12RHI::ResetGraphicsContext(RHIGraphicsContext* pContext)
{
    D3D12GraphicsContext* pD3D12Context = static_cast<D3D12GraphicsContext*>(pContext);
    pD3D12Context->Reset(mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT));
}

void D3D12RHI::SubmitRenderCommands(RHIGraphicsContext* pContext)
{
    D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
    ID3D12GraphicsCommandList* pCommandList = pNativeContext->mCommandList;
    ResourceStateTracker& stateTracker = *pNativeContext->mResourceStateTracker;
    pCommandList->Close();
    std::vector<D3D12_RESOURCE_BARRIER>&& barriers = stateTracker.BuildPreTransitions();
    stateTracker.StopTracking(false);
    const auto& synchronizations = pNativeContext->mSynchronizes;
    for (const auto& synchronization : synchronizations)
    {
        mDirectQueue->Wait(synchronization.first->GetD3D12Fence(), synchronization.second);
    }
    pNativeContext->mSynchronizes.clear();

    if (!barriers.empty())
    {
        ID3D12CommandAllocator* pAllocator = pNativeContext->mCommandAllocator;
        ID3D12GraphicsCommandList* pPreExecution = mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        pPreExecution->Reset(pAllocator, nullptr);
        ID3D12GraphicsCommandList* commandLists[] = { pPreExecution, pCommandList };
        pPreExecution->ResourceBarrier(barriers.size(), barriers.data());
        pPreExecution->Close();
        mDirectQueue->ExecuteCommandLists(2, CommandListCast(commandLists));
        mCommandObjectPool->ReleaseCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, pPreExecution);
    }
    else
    {
        mDirectQueue->ExecuteCommandLists(1, CommandListCast(&pCommandList));
    }
    pNativeContext->mCommandList = nullptr;
    mCommandObjectPool->ReleaseCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandList);
}

void D3D12RHI::SyncGraphicContext(RHIFence* pFence, uint64_t semaphore)
{
    mDirectQueue->Signal(static_cast<D3D12Fence*>(pFence)->GetD3D12Fence(), semaphore);
}

void D3D12RHI::ResetCopyContext(RHICopyContext* pContext) const
{
    D3D12CopyContext* pD3D12Context = static_cast<D3D12CopyContext*>(pContext);
    pD3D12Context->Reset(mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_COPY)); // can we delay synchronize?
}

void D3D12RHI::SubmitCopyCommands(RHICopyContext* pContext)
{
    D3D12CopyContext* pNativeContext = static_cast<D3D12CopyContext*>(pContext);
    ID3D12GraphicsCommandList* pCommandList = pNativeContext->mCommandList;
    pCommandList->Close();
    std::vector<D3D12_RESOURCE_BARRIER>&& barriers = pNativeContext->mResourceStateTracker->BuildPreTransitions();
    pNativeContext->mResourceStateTracker->StopTracking(true);
    const auto& synchronizations = pNativeContext->mSynchronizes;
    for (const auto& synchronization : synchronizations)
    {
        mCopyQueue->Wait(synchronization.first->GetD3D12Fence(), synchronization.second);
    }
    pNativeContext->mSynchronizes.clear();

	if (!barriers.empty())
	{
        ID3D12CommandAllocator* pAllocator = pNativeContext->mCommandAllocator;
        ID3D12GraphicsCommandList* pPreExecution = mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_COPY);
        pPreExecution->Reset(pAllocator, nullptr);
        pPreExecution->ResourceBarrier(barriers.size(), barriers.data());
        pPreExecution->Close();
        ID3D12GraphicsCommandList* commandLists[] = { pPreExecution, pCommandList };
        mCopyQueue->ExecuteCommandLists(2, CommandListCast(commandLists));
        mCommandObjectPool->ReleaseCommandList(D3D12_COMMAND_LIST_TYPE_COPY, pPreExecution);
	}
	else
	{
        mCopyQueue->ExecuteCommandLists(1, CommandListCast(&pCommandList));
	}
    pNativeContext->mCommandList = nullptr;
    mCommandObjectPool->ReleaseCommandList(D3D12_COMMAND_LIST_TYPE_COPY, pCommandList);
}

void D3D12RHI::SyncCopyContext(RHIFence* pFence, uint64_t semaphore) const
{
    mCopyQueue->Signal(static_cast<D3D12Fence*>(pFence)->GetD3D12Fence(), semaphore);
}

void D3D12RHI::BatchCopyCommands(RHICopyContext** pContexts, uint32_t numContexts)
{
    D3D12CopyContext* pNativeContext = static_cast<D3D12CopyContext*>(pContexts[0]);
    ID3D12CommandAllocator* pAllocator = pNativeContext->mCommandAllocator;
    ID3D12GraphicsCommandList** pCommandLists = new ID3D12GraphicsCommandList * [numContexts << 1];
    uint32_t numCommandLists;

    std::vector<D3D12_RESOURCE_BARRIER>&& barriers = pNativeContext->mResourceStateTracker->BuildPreTransitions();
    
    // TODO: batch support
	if (!barriers.empty())
	{
	    pCommandLists[0] = mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_COPY);
	    pCommandLists[0]->Reset(pAllocator, nullptr);
	    pCommandLists[0]->ResourceBarrier(barriers.size(), barriers.data());
	    pCommandLists[0]->Close();
	    pCommandLists[1] = pNativeContext->mCommandList;
        numCommandLists = 2;
	}
	else
	{
	    pCommandLists[0] = pNativeContext->mCommandList;
        numCommandLists = 1;
	}
    pNativeContext->mCommandList = nullptr;

	for (int i = 1; i < numContexts; ++i)
	{
	    ResourceStateTracker& prev = *pNativeContext->mResourceStateTracker;
        pNativeContext = static_cast<D3D12CopyContext*>(pContexts[i]);
	    ResourceStateTracker& next = *pNativeContext->mResourceStateTracker;
	    barriers = prev.Join(next);
	    ID3D12GraphicsCommandList* pCommandList = pNativeContext->mCommandList;
	    pAllocator = pNativeContext->mCommandAllocator;
	    pCommandList->Close();
	    if (!barriers.empty())
	    {
	        pCommandLists[numCommandLists++] = pCommandList;
	        pCommandList = mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_COPY);
	        pCommandList->Reset(pAllocator, nullptr);
	        pCommandList->ResourceBarrier(barriers.size(), barriers.data());
	        pCommandList->Close();
	    }
	    pCommandLists[numCommandLists++] = pCommandList;
        pNativeContext->mCommandList = nullptr;
	}
    pNativeContext->mResourceStateTracker->StopTracking(true);

	mCopyQueue->ExecuteCommandLists(numCommandLists, CommandListCast(pCommandLists));

	for (uint8_t i = 0; i < numCommandLists; ++i)
	{
	    mCommandObjectPool->ReleaseCommandList(D3D12_COMMAND_LIST_TYPE_COPY, pCommandLists[i]);
	}
	delete[] pCommandLists;
}

void D3D12RHI::ReleaseGraphicsContext(RHIGraphicsContext* pContext)
{
    D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
    pNativeContext->Reset(nullptr);
    mCommandObjectPool->ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, pNativeContext->mCommandAllocator);
}

void D3D12RHI::ReleaseCopyContext(RHICopyContext* pContext)
{
    D3D12CopyContext* pNativeContext = static_cast<D3D12CopyContext*>(pContext);
    pNativeContext->Reset(nullptr);
    mCommandObjectPool->ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, pNativeContext->mCommandAllocator);
}

//void D3D12RHI::Present(RHISwapChain* pSwapChain)
//{
//    /*ID3D12CommandAllocator* pAllocator = mCommandObjectPool->ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
//    ID3D12GraphicsCommandList* pCommandList = mCommandObjectPool->ObtainCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
//    pCommandList->Reset(pAllocator, nullptr);
//    ID3D12Resource* pBackBuffer = 
//    D3D12SwapChain* pNativeSwapChain = static_cast<D3D12SwapChain*>(pSwapChain);
//    pNativeSwapChain->Present();*/
//}

void D3D12RHI::Release()
{
    mRTVAllocator.release();
    mDSVAllocator.release();
    mDevice.release();
}

D3D12Device* D3D12RHI::GetD3D12Device() const
{
    return mDevice.get();
}

D3D12RHI::D3D12RHI() = default;

D3D12RHI::~D3D12RHI()
{
    D3D12RHI::Release();
}

void D3D12RHI::GetShaderProperties(ShaderType type, ID3D12ShaderReflection* pReflector, std::unordered_set<ShaderProp>& properties)
{
    D3D12_SHADER_DESC shaderDesc;
    D3D12_SHADER_INPUT_BIND_DESC bindingDesc;
    pReflector->GetDesc(&shaderDesc);
    properties.reserve(shaderDesc.BoundResources);
    for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
    {
        pReflector->GetResourceBindingDesc(i, &bindingDesc);
        ShaderProp property;
        property.mRegister = bindingDesc.BindPoint;
        property.mName = bindingDesc.Name;

        switch (bindingDesc.Type)
        {
        case D3D_SIT_CBUFFER:
            property.mType = ShaderPropType::CBUFFER;
        {
			D3D12_SHADER_BUFFER_DESC bufferDesc;
			pReflector->GetConstantBufferByName(bindingDesc.Name)->GetDesc(&bufferDesc);
			property.mInfo.mCBufferSize = bufferDesc.Size;
        }
            break;
        case D3D_SIT_TEXTURE:
        case D3D_SIT_TBUFFER:
            property.mType = ShaderPropType::TEXTURE;
            property.mInfo.mTextureDimension = ::ConvertFromD3DSRVFormat(bindingDesc.Dimension);
            break;
        case D3D_SIT_SAMPLER:
            property.mType = ShaderPropType::SAMPLER;
            // TODO:
            if (property.mRegister < 4) continue;   // skip static samplers 
            break;
        default:
            break;
        }
        auto it = properties.insert(property);
        // TODO: modify the shader visibility.
    }
}

void D3D12RHI::GetShaderInputElements(ID3D12ShaderReflection* pReflector, std::vector<ShaderInput>& inputElements)
{
    D3D12_SHADER_DESC shaderDesc;
    pReflector->GetDesc(&shaderDesc);
    D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
    inputElements.resize(shaderDesc.InputParameters);
    for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
    {
        pReflector->GetInputParameterDesc(i, &inputDesc);

        inputElements[i] = {};
        inputElements[i].mSemanticName = inputDesc.SemanticName;
        inputElements[i].mSemanticIndex = inputDesc.SemanticIndex;
        inputElements[i].mInputSlot = 0;
        inputElements[i].mFormat = GetFormatFromSignature(inputDesc);
    }
    pReflector->Release();
}

Format D3D12RHI::GetFormatFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc)
{
    switch (paramDesc.ComponentType)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        switch (paramDesc.Mask)
        {
    case 1: return Format::R32_UINT;   
    case 3: return Format::R32G32_UINT;  
    case 7: return Format::R32G32B32_UINT;
    case 15: return Format::R32G32B32A32_UINT;
    default: break;
        }
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        switch (paramDesc.Mask)
        {
    case 1: return Format::R32_SINT;  
    case 3: return Format::R32G32_SINT; 
    case 7: return Format::R32G32B32_SINT;
    case 15: return Format::R32G32B32A32_SINT;
    default: break;
        }
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        switch (paramDesc.Mask)
        {
    case 1: return Format::R32_FLOAT; 
    case 3: return Format::R32G32_FLOAT;
    case 7: return Format::R32G32B32_FLOAT;
    case 15: return Format::R32G32B32A32_FLOAT;
    default: break;
        }
        break;
    default: break;
    }
    return Format::UNKNOWN;
}

std::vector<D3D12_STATIC_SAMPLER_DESC> D3D12RHI::GetStaticSamplers()
{
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

void D3D12RHI::BuildGlobalRootSignature(std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1[]>& ranges, std::vector<CD3DX12_ROOT_PARAMETER1>& params, std::vector<
                                        D3D12_STATIC_SAMPLER_DESC>& samplers) const
{
    const RootSignatureLayout& registerLayout = mConfiguration.mRootSignatureLayout;
    // CreateGlobalRootSignature
    ranges.reset(new CD3DX12_DESCRIPTOR_RANGE1[2]);
    params.resize(3);

    // pass constants
    params[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
    // object transformation
    params[1].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE);
    // | shadow map, depth buffer, opaque buffer.... material textures | material constants | 
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, registerLayout.mNumMaterialConstants, 2);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, registerLayout.mNumTextures, 0);
    params[2].InitAsDescriptorTable(2, ranges.get());
    
    samplers = GetStaticSamplers();
}
#endif