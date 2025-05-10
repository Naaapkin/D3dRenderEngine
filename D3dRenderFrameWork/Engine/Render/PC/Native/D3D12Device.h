#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/Render/PC/D3dUtil.h"
#include "Engine/common/helper.h"

class D3D12Device : NonCopyable
{
public:
    static std::unique_ptr<D3D12Device> CreateD3D12Device();
    
    GUID Guid() const;
    ID3D12Device* GetD3D12Device() const;

    UComPtr<ID3D12Heap>             CreateHeap(const D3D12_HEAP_DESC& desc) const;
    UComPtr<ID3D12Resource>         CreateCommitedResource(const D3D12_HEAP_PROPERTIES& heapProp, const D3D12_HEAP_FLAGS& heapFlags, const D3D12_RESOURCE_DESC& desc, const
                                                           D3D12_CLEAR_VALUE* clearValue, D3D12_RESOURCE_STATES initialState) const;
    UComPtr<ID3D12Resource>         CreatePlacedResource(ID3D12Heap* pHeap, uint64_t offset, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE& clearValue) const;
    UComPtr<ID3D12PipelineState>    CreateGraphicsPipelineStateObject(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) const;
    UComPtr<ID3D12PipelineState>    CreateComputePipelineStateObject(const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) const;
    UComPtr<ID3D12DescriptorHeap>   CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag, uint32_t numDescriptors) const;
    UComPtr<ID3D12RootSignature>    CreateRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const;
    UComPtr<ID3D12CommandQueue>     CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& queueDesc) const;
    UComPtr<ID3D12GraphicsCommandList> CreateCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pAllocator) const;
    UComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type) const;
    UComPtr<ID3D12Fence>            CreateFence() const;
    UComPtr<IDXGISwapChain1>        CreateSwapChain(ID3D12CommandQueue* pQueue, const DXGI_SWAP_CHAIN_DESC1& desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullScreenDesc) const;
    void CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;
    void CreateShaderResourceView(ID3D12Resource* pResource, D3D12_SHADER_RESOURCE_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    void CreateRenderTargetView(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE dst) const;
    void CreateDepthStencilView(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE dst) const;
    
    void Release();

    // ID3D12Heap* CreateCommitedHeap(const D3D12_HEAP_PROPERTIES& heapProp)
    D3D12Device();
    D3D12Device(UComPtr<IDXGIFactory4> pFactory, UComPtr<ID3D12Device> pDevice = nullptr);
    
private:
    static std::vector<D3D12_STATIC_SAMPLER_DESC> GetStaticSamplers();
    
#if defined(DEBUG) or defined(_DEBUG)
    UComPtr<ID3D12InfoQueue> mInfoQueue;
#endif
    UComPtr<IDXGIFactory4> mFactoryHandle;
    UComPtr<ID3D12Device> mDevice;
};

class D3D12DeviceChild : NonCopyable
{
public:
    D3D12Device* Device() const;
    
protected:
    virtual GUID Guid() const = 0;
    
    D3D12DeviceChild(D3D12Device* parent = nullptr);
    D3D12DeviceChild(D3D12DeviceChild&& other) noexcept;
    virtual ~D3D12DeviceChild();

    D3D12DeviceChild& operator=(D3D12DeviceChild&& other) noexcept;
    bool operator==(const D3D12DeviceChild& other) const noexcept;
    bool operator!=(const D3D12DeviceChild& other) const noexcept;
    
private:
    D3D12Device* mDevice;
};

inline GUID D3D12Device::Guid() const
{
    GUID guid;
    mDevice->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline ID3D12Device* D3D12Device::GetD3D12Device() const
{
    return mDevice.Get();
}

inline UComPtr<ID3D12Heap> D3D12Device::CreateHeap(const D3D12_HEAP_DESC& desc) const
{
    return ::CreateDeap(mDevice.Get(), desc);
}

inline UComPtr<ID3D12Resource> D3D12Device::CreateCommitedResource(const D3D12_HEAP_PROPERTIES& heapProp,
                                                                   const D3D12_HEAP_FLAGS& heapFlags,
                                                                   const D3D12_RESOURCE_DESC& desc,
                                                                   const D3D12_CLEAR_VALUE* clearValue,
                                                                   D3D12_RESOURCE_STATES initialState) const
{
    return ::CreateCommitedResource(mDevice.Get(), heapProp, heapFlags, desc, clearValue, initialState);
}

inline UComPtr<ID3D12Resource> D3D12Device::CreatePlacedResource(ID3D12Heap* pHeap, uint64_t offset,
    const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE& clearValue) const
{
    return ::CreatePlacedResource(mDevice.Get(), pHeap, offset, desc, initialState, clearValue);
}

inline UComPtr<ID3D12PipelineState> D3D12Device::CreateGraphicsPipelineStateObject(
    CONST D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc) const
{
    return ::CreateGraphicsPipelineStateObject(mDevice.Get(), desc);
}

inline UComPtr<ID3D12PipelineState> D3D12Device::CreateComputePipelineStateObject(
    const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc) const
{
    return ::CreateComputePipelineStateObject(mDevice.Get(), desc);
}

inline UComPtr<ID3D12DescriptorHeap> D3D12Device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                                       D3D12_DESCRIPTOR_HEAP_FLAGS flag,
                                                                       uint32_t numDescriptors) const
{
    return ::CreateDescriptorHeap(mDevice.Get(), type, flag, numDescriptors);
}

inline UComPtr<ID3D12RootSignature> D3D12Device::CreateRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const
{
    return ::CreateRootSignature(mDevice.Get(), desc);
}

inline UComPtr<ID3D12CommandQueue> D3D12Device::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& queueDesc) const
{
    return ::CreateCommandQueue(mDevice.Get(), queueDesc);
}

inline UComPtr<ID3D12GraphicsCommandList> D3D12Device::CreateCommandList(D3D12_COMMAND_LIST_TYPE type,
                                                                         ID3D12CommandAllocator* pAllocator) const
{
    return ::CreateCommandList(mDevice.Get(), type, pAllocator);
}

inline UComPtr<ID3D12CommandAllocator> D3D12Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type) const
{
    return ::CreateCommandAllocator(mDevice.Get(), type);
}

inline UComPtr<ID3D12Fence> D3D12Device::CreateFence() const
{
    return ::CreateFence(mDevice.Get());
}

inline UComPtr<IDXGISwapChain1> D3D12Device::CreateSwapChain(ID3D12CommandQueue* pQueue, const DXGI_SWAP_CHAIN_DESC1& desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullScreenDesc) const
{
    return ::CreateSwapChainForHwnd(mFactoryHandle.Get(), pQueue, GetActiveWindow(), desc, fullScreenDesc);
}

inline void D3D12Device::CreateConstantBufferView(D3D12_CONSTANT_BUFFER_VIEW_DESC desc,
                                                  D3D12_CPU_DESCRIPTOR_HANDLE handle) const
{
    mDevice->CreateConstantBufferView(&desc, handle);
}

inline void D3D12Device::CreateShaderResourceView(ID3D12Resource* pResource,
                                                  D3D12_SHADER_RESOURCE_VIEW_DESC desc, D3D12_CPU_DESCRIPTOR_HANDLE handle) const
{
    mDevice->CreateShaderResourceView(pResource, &desc, handle);
}

inline void D3D12Device::CreateRenderTargetView(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE dst) const
{
    mDevice->CreateRenderTargetView(pResource, nullptr, dst);
}

inline void D3D12Device::CreateDepthStencilView(ID3D12Resource* pResource, D3D12_CPU_DESCRIPTOR_HANDLE dst) const
{
    mDevice->CreateDepthStencilView(pResource, nullptr, dst);
}

inline void D3D12Device::Release()
{
#if defined(DEBUG) or defined(_DEBUG)
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
#endif
    mDevice.Release();
}

inline D3D12Device::D3D12Device() = default;

inline D3D12Device::D3D12Device(UComPtr<IDXGIFactory4> pFactory, UComPtr<ID3D12Device> pDevice) : mFactoryHandle(std::move(pFactory)), mDevice(std::move(pDevice))
{
#if defined(DEBUG) or defined(_DEBUG)
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(mInfoQueue.GetAddressOf()))))
    {
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif
}

inline std::unique_ptr<D3D12Device> D3D12Device::CreateD3D12Device()
{
    ID3D12Device* pDevice;
    IDXGIFactory4* pFactory;
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
        CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))
    );

    if (FAILED(D3D12CreateDevice(nullptr,   // use primary graphics adapter(GPU)
        D3D_FEATURE_LEVEL_11_0,                     // support DX11
        IID_PPV_ARGS(&pDevice))))
    {
        // 如果失败，尝试使用WARP适配器
        ComPtr<IDXGIAdapter> warpAdapter = nullptr;
        ThrowIfFailed(
            pFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter))
        );

        ThrowIfFailed(
            D3D12CreateDevice(warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&pDevice)
            )
        );
    }

    return std::make_unique<D3D12Device>(pFactory, pDevice);
}

inline std::vector<D3D12_STATIC_SAMPLER_DESC> D3D12Device::GetStaticSamplers()
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
//
//inline D3D12Device* D3D12DeviceChild::Device() const
//{
//    return mDevice;
//}
//
//inline D3D12DeviceChild::D3D12DeviceChild(D3D12Device* parent) : mDevice(parent) { }
//
//inline D3D12DeviceChild::D3D12DeviceChild(D3D12DeviceChild&& other) noexcept : mDevice(other.mDevice)
//{
//    other.mDevice = nullptr;
//}
//
//inline D3D12DeviceChild::~D3D12DeviceChild() = default;
//
//inline D3D12DeviceChild& D3D12DeviceChild::operator=(D3D12DeviceChild&& other) noexcept
//{
//    if (&other != this)
//    {
//        mDevice = other.mDevice;
//        other.mDevice = nullptr;
//    }
//    return *this;
//}
//
//inline bool D3D12DeviceChild::operator==(const D3D12DeviceChild& other) const noexcept
//{
//    return mDevice == other.mDevice;
//}
//
//inline bool D3D12DeviceChild::operator!=(const D3D12DeviceChild& other) const noexcept
//{
//    return mDevice != other.mDevice;
//}
#endif