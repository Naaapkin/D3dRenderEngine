#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "D3D12Device.h"
#include "D3D12Resource.h"
#include "Engine/common/helper.h"
#include "Engine/memory/BuddyAllocator.h"
#include "Engine/memory/LinearAllocator.h"
#include "Engine/memory/RingAllocator.h"

struct Allocation
{
    Allocation() = default;
	Allocation(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress, void* cpuAddress, uint64_t offset, uint64_t size)
		: mGPUAddress(gpuAddress),
		  mCPUAddress(cpuAddress),
		  mOffset(offset),
		  mSize(size)
	{
	}
    virtual ~Allocation() = default;

	D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
    void* mCPUAddress;
    uint64_t mOffset;
    uint64_t mSize;
};

class D3D12LinearBufferAllocator : NonCopyable
{
public:
    void Initialize(D3D12Device* pDevice, uint64_t poolSize = 4ull * 1024 * 1024 /* 4MB default */);
    const D3D12Resource* GetD3D12Resource() const;
    Allocation Allocate(uint64_t size);
    void Reset();
    D3D12LinearBufferAllocator();
    ~D3D12LinearBufferAllocator();

private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12Resource> mResource;
    void* mCPUVirtualAddress;
    D3D12_GPU_VIRTUAL_ADDRESS mBaseGPUVirtualAddress;
    LinearAllocator mLinearAllocator;
};

class D3D12RingBufferAllocator : NonCopyable
{
public:
    void Initialize(D3D12Device* pDevice, uint64_t poolSize = 4ull * 1024 * 1024 /* 4MB default */);
    const D3D12Resource* GetD3D12Resource() const;
    Allocation Allocate(uint64_t size);
    Allocation AllocateAligned(uint64_t size, uint64_t alignment);
    uint64_t GetTotalSize() const;
    void Reset();
    D3D12RingBufferAllocator();
    ~D3D12RingBufferAllocator();

private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12Resource> mResource;
    void* mCPUVirtualAddress;
    D3D12_GPU_VIRTUAL_ADDRESS mBaseGPUVirtualAddress;
    UnsafeRingAllocator mRingAllocator;
};

class D3D12BuddyBufferAllocator : NonCopyable
{
public:
    void Initialize(D3D12Device* pDevice, uint64_t poolSize = 4ull * 1024 * 1024 /* 4MB default */);
    Allocation Allocate(uint64_t size) const;
    void Free(uint64_t offset) const;
    D3D12BuddyBufferAllocator();
    ~D3D12BuddyBufferAllocator();

private:
    void MergeBuddies(uint64_t blockOffset, uint64_t order);
    static uint64_t GetOrder(uint64_t size);

    D3D12Device* mDevice;
    UComPtr<ID3D12Resource> mResource;
    void* mCPUVirtualAddress;
    D3D12_GPU_VIRTUAL_ADDRESS mBaseGPUVirtualAddress;
    BuddyAllocator mBuddyAllocator;
};

inline D3D12BuddyBufferAllocator::D3D12BuddyBufferAllocator() : mDevice(nullptr), mCPUVirtualAddress(nullptr), mBaseGPUVirtualAddress(0)
{ }

inline D3D12BuddyBufferAllocator::~D3D12BuddyBufferAllocator()
{
    if (mResource)
    {
        mResource->Unmap(0, nullptr);
        mResource.Detach()->Release();
    }
}

inline Allocation D3D12BuddyBufferAllocator::Allocate(size_t size) const
{
    // Constant buffers must be 256-byte aligned
    size = ::AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(size);
    uint64_t offset = mBuddyAllocator.Allocate(size);
    if (offset == MAXUINT64) WARN("failed to allocate constant buffer");
    Allocation allocation;
    allocation.mGPUAddress = mBaseGPUVirtualAddress + offset;
    allocation.mCPUAddress = static_cast<uint8_t*>(mCPUVirtualAddress) + offset;
    allocation.mOffset = offset;
    allocation.mSize = size;

    return allocation;
}

inline void D3D12BuddyBufferAllocator::Free(uint64_t offset) const
{
    if (!mBuddyAllocator.Free(offset))
    {
        WARN("fail to release constant buffer at specified offset.");
    }
}

inline void D3D12LinearBufferAllocator::Initialize(D3D12Device* pDevice, uint64_t poolSize)
{
    mDevice = pDevice;
    poolSize = AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(poolSize);
    mLinearAllocator.Initialize(poolSize);
    // Create a committed resource for constant buffers
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = poolSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    mResource = std::make_unique<D3D12Resource>(mDevice->CreateCommitedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, nullptr, D3D12_RESOURCE_STATE_GENERIC_READ));
    // Map the entire resource
    if (FAILED(mResource->D3D12ResourcePtr()->Map(0, nullptr, &mCPUVirtualAddress)))
    {
        mResource->D3D12ResourcePtr()->Release();
        THROW_EXCEPTION(TEXT("Failed to map constant buffer resource"));
    }

    mBaseGPUVirtualAddress = mResource->GetGPUVirtualAddress();
}

inline const D3D12Resource* D3D12LinearBufferAllocator::GetD3D12Resource() const
{ return mResource.get(); }

inline Allocation D3D12LinearBufferAllocator::Allocate(uint64_t size)
{
    // Constant buffers must be 256-byte aligned
    uint64_t offset = mLinearAllocator.Allocate(size);
    if (offset == MAXUINT64) WARN("failed to allocate constant buffer");
    Allocation allocation;
    allocation.mGPUAddress = mBaseGPUVirtualAddress + offset;
    allocation.mCPUAddress = static_cast<uint8_t*>(mCPUVirtualAddress) + offset;
    allocation.mOffset = offset;
    allocation.mSize = size;

    return allocation;
}

inline void D3D12LinearBufferAllocator::Reset()
{
    mLinearAllocator.Reset();
}

inline D3D12LinearBufferAllocator::D3D12LinearBufferAllocator() = default;

inline D3D12LinearBufferAllocator::~D3D12LinearBufferAllocator()
{
    if (mResource)
    {
        mResource->D3D12ResourcePtr()->Unmap(0, nullptr);
        mResource.reset();
    }
}

inline void D3D12RingBufferAllocator::Initialize(D3D12Device* pDevice, uint64_t poolSize)
{
    mDevice = pDevice;
    poolSize = AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(poolSize);
    mRingAllocator.Initialize(poolSize);

    // Create a committed resource for constant buffers
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = poolSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    mResource = std::make_unique<D3D12Resource>(mDevice->CreateCommitedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, nullptr, D3D12_RESOURCE_STATE_GENERIC_READ));

    // Map the entire resource
    if (FAILED(mResource->D3D12ResourcePtr()->Map(0, nullptr, &mCPUVirtualAddress)))
    {
        mResource->D3D12ResourcePtr()->Release();
        THROW_EXCEPTION(TEXT("Failed to map constant buffer resource"));
    }

    mBaseGPUVirtualAddress = mResource->GetGPUVirtualAddress();
}

inline const D3D12Resource* D3D12RingBufferAllocator::GetD3D12Resource() const
{ return mResource.get(); }

inline Allocation D3D12RingBufferAllocator::Allocate(uint64_t size)
{
    // Constant buffers must be 256-byte aligned
    uint64_t offset = mRingAllocator.Allocate(size);
    if (offset == MAXUINT64) WARN("failed to allocate constant buffer");
    Allocation allocation;
    allocation.mGPUAddress = mBaseGPUVirtualAddress + offset;
    allocation.mCPUAddress = static_cast<uint8_t*>(mCPUVirtualAddress) + offset;
    allocation.mOffset = offset;
    allocation.mSize = size;

    return allocation;
}

inline Allocation D3D12RingBufferAllocator::AllocateAligned(uint64_t size, uint64_t alignment)
{
    // Constant buffers must be 256-byte aligned
    uint64_t offset = mRingAllocator.AllocateAligned(size, alignment);
    if (offset == MAXUINT64) WARN("failed to allocate constant buffer");
    Allocation allocation;
    allocation.mGPUAddress = mBaseGPUVirtualAddress + offset;
    allocation.mCPUAddress = static_cast<uint8_t*>(mCPUVirtualAddress) + offset;
    allocation.mOffset = offset;
    allocation.mSize = size;

    return allocation;
}

inline uint64_t D3D12RingBufferAllocator::GetTotalSize() const
{
    return mRingAllocator.GetTotalSize();
}

inline void D3D12RingBufferAllocator::Reset()
{
    mRingAllocator.Reset();
}

inline D3D12RingBufferAllocator::D3D12RingBufferAllocator() = default;

inline D3D12RingBufferAllocator::~D3D12RingBufferAllocator()
{
    if (mResource)
    {
        mResource->D3D12ResourcePtr()->Unmap(0, nullptr);
        mResource.reset();
    }
}

inline void D3D12BuddyBufferAllocator::Initialize(D3D12Device* pDevice, uint64_t poolSize)
{
    mDevice = pDevice;
    poolSize = AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(poolSize);
    mBuddyAllocator.Initialize(poolSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

    // Create a committed resource for constant buffers
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = poolSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps;
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    mResource = mDevice->CreateCommitedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, nullptr, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Map the entire resource
    if (FAILED(mResource->Map(0, nullptr, &mCPUVirtualAddress)))
    {
        mResource->Release();
        throw std::runtime_error("Failed to map constant buffer resource");
    }

    mBaseGPUVirtualAddress = mResource->GetGPUVirtualAddress();
}
#endif