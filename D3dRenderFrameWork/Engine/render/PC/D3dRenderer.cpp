#ifdef WIN32
#include <Engine/common/Exception.h>
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/Shader.h>
#include <Engine/render/PC/Core/D3dCommandList.h>
#include <Engine/render/PC/Core/D3dGraphicContext.h>
#include <Engine/render/PC/Core/DescriptorHeap.h>
#include <Engine/render/PC/ScriptableRenderPath/RenderPath.h>
#include <Engine/render/PC/RenderResource/FrameResource.h>
#include <Engine/render/PC/RenderResource/RenderTexture.h>
#include <Engine/Window/Frame.h>
#include <Engine/Window/WFrame.h>

const Shader& TempOpaqueVs()
{
    static Shader opaqueVS = Shader::LoadCompiledShader(L"D3dRenderFrameWork/assets/Shaders/PixelShader/vertex.hlsl",
                                                        ShaderType::VERTEX_SHADER);
    return opaqueVS;
}

const Shader& TempOpaquePs()
{
    static Shader opaquePS = Shader::LoadCompiledShader(L"D3dRenderFrameWork/assets/Shaders/VertexShader/pixel.hlsl",
                                                        ShaderType::PIXEL_SHADER);
    return opaquePS;
}

Renderer* gRenderer()
{
    return ::gNativeRenderer();
}

Renderer::~Renderer() = default;

D3dRenderer* gNativeRenderer()
{
    static D3dRenderer renderer{};
    if (D3dRenderer::sGraphicContext) return &renderer;
    D3dRenderer::sGraphicContext.reset(new D3dGraphicContext());
    renderer.initialize();
    return &renderer;
}

D3dGraphicContext const* gGraphicContext()
{
#ifdef DEBUG | _DEBUG
    ASSERT(D3dRenderer::sGraphicContext, TEXT("graphic context is not initialized\n"));
#endif
    return D3dRenderer::sGraphicContext.get();
}

void D3dRenderer::createCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        sGraphicContext->DeviceHandle()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue))
    );
}

Fence D3dRenderer::sCreateFence()
{
    ID3D12Fence* fence;
    if (sGraphicContext) return {};
    ThrowIfFailed(
        sGraphicContext->DeviceHandle()->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))
    );
    return {fence};
}

DescriptorHeap D3dRenderer::sCreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint16_t size)
{
    ID3D12DescriptorHeap* descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.NumDescriptors = size;
    heapDesc.Type = type;
    heapDesc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;
    ThrowIfFailed(
        sGraphicContext->DeviceHandle()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap))
    );
    return {descriptorHeap, size};
}

D3dRenderer::~D3dRenderer()
{
    delete[] mFrameResources;
}

void D3dRenderer::createSwapChainForHwnd()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; swapChainDesc.Height = 0;  // use 0 to fetch the current window size
    swapChainDesc.Format = mBackBufferFormat;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = mNumBackbuffer;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;
    ThrowIfFailed(
        sGraphicContext->FactoryHandle()->CreateSwapChainForHwnd(
            mCommandQueue.Get(),
            dynamic_cast<WFrame&>(gFrame()).WindowHandle(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &mSwapChain)
    );
}

void D3dRenderer::createBackBuffersAndDesc()
{
    if (mFrameResources) return;
    ID3D12Device* pDevice = sGraphicContext->DeviceHandle();
    mFrameResources = new std::unique_ptr<FrameResource>[mNumBackbuffer];
    auto descHandle = mRtDescHeap->HeapHandle()->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < mNumBackbuffer; ++i)
    {
        ID3D12Resource* backBuffer = nullptr;
        ThrowIfFailed(
            mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer))
        );
        ResourceState state = ResourceState::COMMON;
        pDevice->CreateRenderTargetView(backBuffer, nullptr, descHandle);
        descHandle.ptr += mRtDescHeap->DescriptorSize();
        mFrameResources[i] = std::make_unique<FrameResource>(::gCreateFrameResource({ TextureType::TEXTURE_2D, { backBuffer, 1, &state } }));
    }
    mCurrentBackBufferIndex = 0;
}

void D3dRenderer::createDepthStencilAndDesc()
{
#ifdef DEBUG | _DEBUG
    ASSERT(mSwapChain, L"Swap chain is null");
#endif
    DXGI_SWAP_CHAIN_DESC1 desc;
    mSwapChain->GetDesc1(&desc);

    const D3D12_RESOURCE_DESC dsDesc = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0, desc.Width, desc.Height, 1, 1,
        mDepthStencilFormat,
        1, 0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    };

    const D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    const D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(mDepthStencilFormat, 1.0f, 0);
    ThrowIfFailed(sGraphicContext->DeviceHandle()->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &dsDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
    ));
    
    sGraphicContext->DeviceHandle()->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        nullptr,                                    // use nullptr to fetch the depth stencil view matching current hwnd
        mDsDescHeap->HeapHandle()->GetCPUDescriptorHandleForHeapStart()
    );
}

void D3dRenderer::createDescriptorHeap()
{
    mRtDescHeap.reset(new DescriptorHeap{sCreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        MAX_RENDER_TARGET_COUNT)});
    mDsDescHeap.reset(new DescriptorHeap{sCreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        DEPTH_STENCIL_COUNT)});
    mCbSrUaDescHeap.reset(new DescriptorHeap(D3dRenderer::sCreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        MAX_CB_SR_UA_RESOURCE_COUNT)));
}

// TODO: consider using rttr to register all derived render paths
void D3dRenderer::registerRenderPaths()
{
    RenderPath* pRenderPath = new SampleRenderPath();
    FrameResourceAllocator frameResourceAllocator;
    pRenderPath->setUp(frameResourceAllocator);
    mRenderPaths.insert(new SampleRenderPath());
    // mRenderPaths.insert(new ForwardRenderPath());
    for (int i = 0; i < mNumBackbuffer; ++i)
    {
        frameResourceAllocator.ExecutePendingAllocationsForFrame(*mFrameResources[i].get());
    }
}

void D3dRenderer::initialize()
{
    createDescriptorHeap();
    createCommandQueue();
    createSwapChainForHwnd();
    createBackBuffersAndDesc();
    createDepthStencilAndDesc();
    registerRenderPaths();
}

void D3dRenderer::render()
{
    FrameResource& frameResource = *mFrameResources[mCurrentBackBufferIndex];
    
    static D3dCommandList cmdList{::CreateD3dCommandList(sGraphicContext->DeviceHandle(), frameResource.mCommandAllocator.Get(), D3D12_COMMAND_LIST_TYPE_DIRECT)};
    // RenderContext rc(mCommandQueue.Get());
    for (auto renderPath : mRenderPaths)
    {
        renderPath->render(cmdList);
    }
    mCurrentBackBufferIndex = (mCurrentBackBufferIndex + 1) % mNumBackbuffer;
}

D3dRenderer::D3dRenderer() = default;
#endif

std::unique_ptr<D3dGraphicContext> D3dRenderer::sGraphicContext = nullptr;