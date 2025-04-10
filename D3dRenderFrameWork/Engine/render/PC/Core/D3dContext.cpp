#include "D3dCommandList.h"
#ifdef WIN32
#include "Engine/render/PC/Core/D3dContext.h"

#include "Engine/common/Exception.h"

ID3D12Device* D3dContext::deviceHandle() const
{
    return mDeviceHandle.Get();
}

IDXGIFactory4* D3dContext::factoryHandle() const
{
    return mFactoryHandle.Get();
}

Fence D3dContext::createFence(uint64_t initValue) const
{
    ID3D12Fence* pFence;
    mDeviceHandle->CreateFence(initValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence));
    return { pFence };
}

ID3D12CommandQueue* D3dContext::createCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
    ID3D12CommandQueue* pCmdQueue;
    D3D12_COMMAND_QUEUE_DESC queueDesc{};
    queueDesc.Type = type;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(
        mDeviceHandle->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCmdQueue))
    );
    return pCmdQueue;
}

ID3D12CommandAllocator* D3dContext::createCommandAllocator(D3D12_COMMAND_LIST_TYPE type) const
{
    ID3D12CommandAllocator* pCmdAllocator;
    mDeviceHandle->CreateCommandAllocator(type, IID_PPV_ARGS(&pCmdAllocator));
    return pCmdAllocator;
}

ID3D12GraphicsCommandList* D3dContext::createCommandList(D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* pAllocator) const
{
    ID3D12GraphicsCommandList* pCmdList;
    ThrowIfFailed(mDeviceHandle->CreateCommandList(0, type, pAllocator, nullptr, IID_PPV_ARGS(&pCmdList)));
    return pCmdList;
}

DescriptorHeap* D3dContext::createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint16_t size) const
{
    ID3D12DescriptorHeap* descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.NumDescriptors = size;
    heapDesc.Type = type;
    heapDesc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? 
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;
    ThrowIfFailed(
        mDeviceHandle->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&descriptorHeap))
    );
    return new DescriptorHeap{descriptorHeap, mDeviceHandle->GetDescriptorHandleIncrementSize(type), size};
}

ID3D12PipelineState* D3dContext::createPipelineStateObject(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) const
{
    ID3D12PipelineState* pso;
    HRESULT hr = mDeviceHandle->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr))
    {
        OutputDebugStringW(WFunc::GetHRInfo(hr));
    }
    return pso;
}

IDXGISwapChain1* D3dContext::createSwapChainForHwnd(ID3D12CommandQueue* pCommandQueue, Format backBufferFormat, uint8_t numBackBuffer) const
{
    IDXGISwapChain1* pSwapChain;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = 0; swapChainDesc.Height = 0;  // use 0 to fetch the current window size
    swapChainDesc.Format = static_cast<DXGI_FORMAT>(backBufferFormat);
    swapChainDesc.Stereo = false;
    swapChainDesc.SampleDesc = {1, 0};
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = numBackBuffer;
    swapChainDesc.Scaling = DXGI_SCALING_NONE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = 0;
    ThrowIfFailed(
        mFactoryHandle->CreateSwapChainForHwnd(
            pCommandQueue,
            GetActiveWindow(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &pSwapChain)
    );
    return pSwapChain;
}

void D3dContext::initialize()
{

#if defined(DEBUG) or defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();

        // 启用 GPU 验证
        ComPtr<ID3D12Debug1> debugController1;
        if (SUCCEEDED(debugController.As(&debugController1)))
        {
            debugController1->SetEnableGPUBasedValidation(true);
            // 设置更严格的验证
            debugController1->SetEnableSynchronizedCommandQueueValidation(true);
        }
    }
#endif
    ThrowIfFailed(
        CreateDXGIFactory1(IID_PPV_ARGS(&mFactoryHandle))
    );

    if (FAILED(D3D12CreateDevice(nullptr,   // 使用主显示器适配器
        D3D_FEATURE_LEVEL_11_0,                     // 最低支持DX11
        IID_PPV_ARGS(&mDeviceHandle))))
    {
        // 如果失败，尝试使用WARP适配器
        ComPtr<IDXGIAdapter> warpAdapter = nullptr;
        ThrowIfFailed(
            mFactoryHandle->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))
        );

        ThrowIfFailed(
            D3D12CreateDevice(warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&mDeviceHandle)
            )
        );
    }
    
#if defined(DEBUG) or defined(_DEBUG)
    if (SUCCEEDED(mDeviceHandle->QueryInterface(IID_PPV_ARGS(&mInfoQueue))))
    {
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif
}

D3dContext::D3dContext() = default;

ID3D12RootSignature* D3dContext::createRootSignature(ID3DBlob* binary) const
{
    ID3D12RootSignature* pRootSig;
    ThrowIfFailed(mDeviceHandle->CreateRootSignature(0,
        binary->GetBufferPointer(),
        binary->GetBufferSize(),
        IID_PPV_ARGS(&pRootSig))
    );
    return pRootSig;
}

D3dContext& D3dContext::instance()
{
    static D3dContext context{};
    return context;
}

#if defined(DEBUG) or defined(_DEBUG)
D3dContext::~D3dContext()
{
    
    // check for graphic memory leaks
    HMODULE dxgiDebugModule = GetModuleHandleA("Dxgidebug.dll");
    if (!dxgiDebugModule) dxgiDebugModule = LoadLibraryA("Dxgidebug.dll");
    if (dxgiDebugModule != nullptr) {
        // fail to load Dxgidebug.dll
        typedef HRESULT(WINAPI* DXGIGetDebugInterface_t)(REFIID, void**);
        DXGIGetDebugInterface_t DXGIGetDebugInterface = reinterpret_cast<DXGIGetDebugInterface_t>(
            GetProcAddress(dxgiDebugModule, "DXGIGetDebugInterface")
            );
        if (DXGIGetDebugInterface == nullptr) {
            // fail to get the address of func "DXGIGetDebugInterface"
            FreeLibrary(dxgiDebugModule);
            return;
        }
        IDXGIDebug* pDxgiDebug;
        HRESULT hr = DXGIGetDebugInterface(IID_PPV_ARGS(&pDxgiDebug));
        if (SUCCEEDED(hr)) {
            pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            pDxgiDebug->Release();
        }
    }
}
#else
D3dGraphicContext::~D3dGraphicContext() = default;
#endif

#endif
