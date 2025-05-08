#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/Exception.h"

class Blob;

template<typename T, typename T0 = std::enable_if_t<std::is_base_of_v<IUnknown, T>>>
class UComPtr
{
public:
    T* Get() const;
    T* Reset(T* ptr);
    T** GetAddressOf();
    T* const* GetAddressOf() const;

    T* Detach();
    void Release();
    UComPtr();
    UComPtr(T* ptr);
    UComPtr(UComPtr&& other) noexcept;
    ~UComPtr();

    T* operator->() const;
    operator bool() const;
    UComPtr& operator=(UComPtr&& other) noexcept;

    NON_COPYABLE(UComPtr)

private:
    T* mPtr;
};

template <typename T, typename T0>
T* UComPtr<T, T0>::Get() const
{ return mPtr; }

template <typename T, typename T0>
T* UComPtr<T, T0>::Reset(T* ptr)
{ std::swap(mPtr, ptr); return ptr; }

template <typename T, typename T0>
T** UComPtr<T, T0>::GetAddressOf()
{ return &mPtr; }

template <typename T, typename T0>
T* const* UComPtr<T, T0>::GetAddressOf() const
{ return &mPtr; }

template <typename T, typename T0>
T* UComPtr<T, T0>::Detach()
{
    T* ptr = mPtr;
    mPtr = nullptr;
    return ptr;
}

template <typename T, typename T0>
void UComPtr<T, T0>::Release()
{ if (mPtr) {mPtr->Release(); mPtr = nullptr; } }

template <typename T, typename T0>
UComPtr<T, T0>::UComPtr(): mPtr(nullptr)
{}

template <typename T, typename T0>
UComPtr<T, T0>::UComPtr(T* ptr): mPtr(ptr)
{}

template <typename T, typename T0>
UComPtr<T, T0>::UComPtr(UComPtr&& other) noexcept: mPtr(other.mPtr)
{ other.mPtr = nullptr; }

template <typename T, typename T0>
UComPtr<T, T0>::~UComPtr()
{ Release(); }

template <typename T, typename T0>
T* UComPtr<T, T0>::operator->() const
{ return mPtr; }

template <typename T, typename T0>
UComPtr<T, T0>::operator bool() const
{ return mPtr; }

template <typename T, typename T0>
UComPtr<T, T0>& UComPtr<T, T0>::operator=(UComPtr&& other) noexcept
{
    Release();
    mPtr = other.mPtr;
    other.mPtr = nullptr;
    return *this;
}

// ban array and array pointer type
template<typename T>
class UComPtr<T[]>;

template<typename T, std::size_t N>
class UComPtr<T[N]>;

template<typename T>
struct alignas(256) Constant
{
    T value;

    Constant(T value) : value(value) { }
    operator T() const { return value; }
    T* operator->() { return value.operator->(); }
};

struct PassData
{
    // light info
    DirectX::XMVECTOR mLightDirection;
    DirectX::XMVECTOR mLightIntensity;
    DirectX::XMVECTOR mLightColor;
    DirectX::XMVECTOR mAmbientColor;
};

inline UComPtr<ID3D12Heap> CreateDeap(ID3D12Device* pDevice, const D3D12_HEAP_DESC& desc)
{
    ID3D12Heap* pHeap;
    pDevice->CreateHeap(&desc, IID_PPV_ARGS(&pHeap));
    return pHeap;
}

inline UComPtr<ID3D12Resource> CreatePlacedResource(ID3D12Device* pDevice, ID3D12Heap* pHeap, uint64_t offset, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE& clearValue)
{
    ID3D12Resource* pResource;
    pDevice->CreatePlacedResource(pHeap, offset, &desc, initialState, &clearValue, IID_PPV_ARGS(&pResource));
    return pResource;
}

inline UComPtr<ID3D12Resource> CreateCommitedResource(ID3D12Device* pDevice, const D3D12_HEAP_PROPERTIES& heapProp,
                                                      const D3D12_HEAP_FLAGS& heapFlags, const D3D12_RESOURCE_DESC& desc, const D3D12_CLEAR_VALUE* clearValue,
                                                      D3D12_RESOURCE_STATES initialState)
{
    ID3D12Resource* pResource;
    pDevice->CreateCommittedResource(&heapProp, heapFlags, &desc, initialState, clearValue, IID_PPV_ARGS(&pResource));
    return pResource;
}

inline UComPtr<ID3D12PipelineState> CreateGraphicsPipelineStateObject(ID3D12Device* pDevice,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
{
    ID3D12PipelineState* pipelineState;
    ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
    return pipelineState;
}

inline UComPtr<ID3D12PipelineState> CreateComputePipelineStateObject(ID3D12Device* pDevice,
    const D3D12_COMPUTE_PIPELINE_STATE_DESC& desc)
{
    ID3D12PipelineState* pipelineState;
    ThrowIfFailed(pDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
    return pipelineState;
}

inline UComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_DESCRIPTOR_HEAP_FLAGS flag, uint32_t numDescriptors)
{
    ID3D12DescriptorHeap* pHeap;
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Flags = flag;
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.NodeMask = 0;
    ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
    return pHeap;
}

inline UComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* pDevice, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
{
    ID3D12RootSignature* pRootSignature;
    UComPtr<ID3DBlob> blob;
    UComPtr<ID3DBlob> errorBlob;
    if (FAILED((D3D12SerializeVersionedRootSignature(&desc, blob.GetAddressOf(), errorBlob.GetAddressOf()))))
    {
        OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
        THROW_EXCEPTION(TEXT("Failed to create root signature"));
    }
    ThrowIfFailed(pDevice->CreateRootSignature(0, blob->GetBufferPointer(),
        blob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
    return pRootSignature;
}

inline UComPtr<ID3D12CommandQueue> CreateCommandQueue(ID3D12Device* pDevice, const D3D12_COMMAND_QUEUE_DESC& queueDesc)
{
    ID3D12CommandQueue* pCommandQueue;
    ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));
    return pCommandQueue;
}

inline UComPtr<ID3D12GraphicsCommandList> CreateCommandList(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* pAllocator)
{
    ID3D12GraphicsCommandList* pCommandList;
    ThrowIfFailed(pDevice->CreateCommandList(0, type, pAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));
    return pCommandList;
}

inline UComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
{
    ID3D12CommandAllocator* pCommandAllocator;
    ThrowIfFailed(pDevice->CreateCommandAllocator(type, IID_PPV_ARGS(&pCommandAllocator)));
    return pCommandAllocator;
}

inline UComPtr<ID3D12Fence> CreateFence(ID3D12Device* pDevice)
{
    ID3D12Fence* pFence;
    ThrowIfFailed(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));
    return pFence;
}

inline UComPtr<IDXGISwapChain1> CreateSwapChainForHwnd(IDXGIFactory4* pFactory, ID3D12CommandQueue* pQueue, HWND windowHandle, const DXGI_SWAP_CHAIN_DESC1& desc, const
                                                       DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullScreenDesc)
{
    IDXGISwapChain1* pSwapChain;
    ThrowIfFailed(pFactory->CreateSwapChainForHwnd(pQueue, windowHandle, &desc, pFullScreenDesc, nullptr, &pSwapChain));
    return pSwapChain;
}

DXGI_FORMAT GetParaInfoFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
D3D12_GRAPHICS_PIPELINE_STATE_DESC DefaultPipelineStateDesc();
uint64_t GetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::string& name);
uint64_t GetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::wstring& name);
bool ImplicitTransition(uint32_t stateBefore, uint32_t& stateAfter, bool isBufferOrSimultaneous);
UComPtr<ID3DBlob> D3D12Compile(const Blob& blob, const char* entry, const char* pTarget, const char* file);
void CopyTextureDataWithPitchAlignment(
	void* pDest,
	// 上传堆中映射的内存地址（起始指针）
	const void* pSrc,
	// 原始图像数据（每行连续）
	uint32_t rowSize,
	// 纹理宽度（像素）
	uint32_t height
);
#endif
