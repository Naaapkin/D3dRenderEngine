#ifdef WIN32
#include <Engine/common/PC/WException.h>
#include <Engine/render/PC/D3dCommandList.h>
#include <Engine/render/PC/D3dGraphicContext.h>
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/DescriptorHeap.h>
#include <Engine/render/PC/FrameResource.h>
#include <Engine/render/PC/Shader.h>
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
    if (D3dRenderer::sRenderContext) return &renderer;
    D3dRenderer::sRenderContext.reset(new D3dGraphicContext());
    renderer.Initialize();
    return &renderer;
}

D3dGraphicContext const* gGraphicContext()
{
#ifdef DEBUG | _DEBUG
    if (!D3dRenderer::sRenderContext)
    {
        RY_WARN(TEXT("d3d renderer is not initialized"));
    }
#endif
    return D3dRenderer::sRenderContext.get();
}

void D3dRenderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        sRenderContext->DeviceHandle()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue))
    );
}

Fence D3dRenderer::sCreateFence()
{
    ID3D12Fence* fence;
    if (sRenderContext) return {};
    ThrowIfFailed(
        sRenderContext->DeviceHandle()->CreateFence(
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
        sRenderContext->DeviceHandle()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap))
    );
    return {descriptorHeap, size};
}

D3dRenderer::~D3dRenderer()
{
    delete[] mBackBuffers;
    delete[] mFrameResources;
}

void D3dRenderer::LoadShaders()
{
    // TODO: load and compile shaders
    
}

void D3dRenderer::CreateSwapChainForHwnd()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; swapChainDesc.Height = 0;  // use 0 ti fetch the current window size
    swapChainDesc.Format = mBackBufferFormat;
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = mBackbufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;
    ThrowIfFailed(
        sRenderContext->FactoryHandle()->CreateSwapChainForHwnd(
            mCommandQueue.Get(),
            sFrame->WindowHandle(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &mSwapChain)
    );
}

void D3dRenderer::CreateBackBuffersAndDesc()
{
    if (!mBackBuffers) return;
    ID3D12Device* pDevice = sRenderContext->DeviceHandle();
    mBackBuffers = new ComPtr<ID3D12Resource>[mBackbufferCount];
    auto descHandle = mRtDescHeap->HeapHandle()->GetCPUDescriptorHandleForHeapStart();
    for (uint8_t i = 0; i < mBackbufferCount; i++)
    {
        ThrowIfFailed(
            mSwapChain->GetBuffer(i, IID_PPV_ARGS((mBackBuffers + i)->GetAddressOf()))
        );
        pDevice->CreateRenderTargetView((mBackBuffers + i)->Get(), nullptr, descHandle);
        descHandle.ptr += mRtDescHeap->DescriptorSize();
    }
    mCurrentBackBufferIndex = 0;
}

void D3dRenderer::CreateDepthStencilAndDesc()
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
    ThrowIfFailed(sRenderContext->DeviceHandle()->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &dsDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())
    ));
    
    sRenderContext->DeviceHandle()->CreateDepthStencilView(
        mDepthStencilBuffer.Get(),
        nullptr,                                    // use nullptr to fetch the depth stencil view matching current hwnd
        mDsDescHeap->HeapHandle()->GetCPUDescriptorHandleForHeapStart()
    );
}

void D3dRenderer::CreateFrameResources()
{
    if (!mFrameResources) return;
    mFrameResources = new std::unique_ptr<FrameResource>[mBackbufferCount];
    for (int i = 0; i < mBackbufferCount; ++i)
    {
        mFrameResources[i] = std::make_unique<FrameResource>(::CreateFrameResource(PASS_COUNT, OBJECT_COUNT));
    }
}

void D3dRenderer::CreateRootSignature()
{
    CD3DX12_DESCRIPTOR_RANGE1 range;
    range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0);
    CD3DX12_ROOT_PARAMETER1 rootParam;
    rootParam.InitAsDescriptorTable(1, &range);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    desc.Init_1_1(1, &rootParam, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* blob;
    ID3DBlob* errorBlob;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&desc, &blob, &errorBlob));
    ThrowIfFailed(sRenderContext->DeviceHandle()->CreateRootSignature(0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&mRootSignature))
    );
    blob->Release();
}

void D3dRenderer::Initialize()
{
    sFrame.reset(reinterpret_cast<WFrame*>(
       IFrame::CreateFrame(TEXT("D3dRenderFramework"), 1280, 720)
       ));
#ifdef DEBUG | _DEBUG
    ASSERT(sFrame, TEXT("Failed to create win frame"));
#endif
    
    CreateDescriptorHeap();
    CreateCommandQueue();
    CreateSwapChainForHwnd();
    CreateBackBuffersAndDesc();
    CreateDepthStencilAndDesc();
    CreateFrameResources();
    CreateRootSignature();
    LoadShaders();
}

void D3dRenderer::CreateDescriptorHeap()
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

D3dRenderer::D3dRenderer() = default;
#endif

std::unique_ptr<D3dGraphicContext> D3dRenderer::sRenderContext = nullptr;
std::unique_ptr<WFrame> D3dRenderer::sFrame = nullptr;