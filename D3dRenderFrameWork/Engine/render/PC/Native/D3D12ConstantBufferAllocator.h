#pragma once
#include "Engine/pch.h"
#include "Engine/render/PC/Native/D3D12Device.h"
#undef min
#undef max

class D3D12ConstantBufferAllocator : D3D12DeviceChild
{
public:
    struct Allocation
    {
        D3D12_GPU_VIRTUAL_ADDRESS mGPUAddress;
        void* mCPUAddress;
        uint64_t mOffset;
        uint64_t mSize;
    };

    GUID Guid() const override;
    
    void Initialize(uint64_t poolSize = 4 * 1024 * 1024 /* 4MB default */);
    Allocation Allocate(uint64_t poolSize);
    void Free(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    D3D12ConstantBufferAllocator(D3D12Device* pParent);
    ~D3D12ConstantBufferAllocator() override;

private:
    void MergeBuddies(uint64_t blockOffset, uint64_t order);
    static uint64_t GetOrder(uint64_t size);

    UComPtr<ID3D12Resource> mResource;
    void* mCpuVirtualAddress;
    D3D12_GPU_VIRTUAL_ADDRESS mGpuVirtualAddress;
    uint64_t mPoolSize;
    uint64_t mMaxOrder;
    std::vector<std::vector<uint64_t>> mFreeLists;
    std::unordered_map<D3D12_GPU_VIRTUAL_ADDRESS, Allocation> mAllocations;
};

inline D3D12ConstantBufferAllocator::D3D12ConstantBufferAllocator(D3D12Device* pParent) : D3D12DeviceChild(pParent), mCpuVirtualAddress(nullptr), mGpuVirtualAddress(0), mPoolSize(0), mMaxOrder(0)
{ }

inline D3D12ConstantBufferAllocator::~D3D12ConstantBufferAllocator()
{
    if (mResource)
    {
        mResource->Unmap(0, nullptr);
        mResource->Release();
    }
}

inline D3D12ConstantBufferAllocator::Allocation D3D12ConstantBufferAllocator::Allocate(size_t poolSize)
{
    // Constant buffers must be 256-byte aligned
    poolSize = ::AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(poolSize);

    // Find the smallest free block that can satisfy the request
    size_t order = GetOrder(poolSize);
    size_t foundOrder = order;
        
    while (foundOrder <= mMaxOrder && mFreeLists[foundOrder].empty())
    {
        foundOrder++;
    }

    if (foundOrder > mMaxOrder) return { MAXUINT64, nullptr };

    // Split blocks until we reach the desired order
    while (foundOrder > order)
    {
        size_t blockOffset = mFreeLists[foundOrder].back();
        mFreeLists[foundOrder].pop_back();

        foundOrder--;
        size_t buddyOffset = blockOffset + (1 << foundOrder);

        mFreeLists[foundOrder].push_back(blockOffset);
        mFreeLists[foundOrder].push_back(buddyOffset);
    }

    // Allocate the block
    size_t blockOffset = mFreeLists[order].back();
    mFreeLists[order].pop_back();

    Allocation allocation;
    allocation.mGPUAddress = mGpuVirtualAddress + blockOffset;
    allocation.mCPUAddress = static_cast<uint8_t*>(mCpuVirtualAddress) + blockOffset;
    allocation.mOffset = blockOffset;
    allocation.mSize = poolSize;

    mAllocations[allocation.mGPUAddress] = allocation;

    return allocation;
}

inline void D3D12ConstantBufferAllocator::Free(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
    auto it = mAllocations.find(gpuAddress);
    if (it == mAllocations.end())
    {
        throw std::runtime_error("Attempting to free unknown constant buffer");
    }

    Allocation allocation = it->second;
    mAllocations.erase(it);

    // Calculate order from mSize (rounded to power of two)
    size_t roundedSize = NextPowerOfTwo(allocation.mSize);
    size_t order = static_cast<size_t>(log2(roundedSize));

    // Add to free list and try to merge buddies
    mFreeLists[order].push_back(allocation.mOffset);
    MergeBuddies(allocation.mOffset, order);
}

inline GUID D3D12ConstantBufferAllocator::Guid() const
{
    GUID guid;
    mResource->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline void D3D12ConstantBufferAllocator::Initialize(uint64_t poolSize)
{
    mPoolSize = AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(poolSize);
    mMaxOrder = static_cast<size_t>(log2(mPoolSize));

    // Create a committed resource for constant buffers
    D3D12_RESOURCE_DESC desc;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = mPoolSize;
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

    mResource = Device()->CreateCommitedResource(heapProps, D3D12_HEAP_FLAG_NONE, desc, CD3DX12_CLEAR_VALUE{}, D3D12_RESOURCE_STATE_GENERIC_READ);

    // Map the entire resource
    if (FAILED(mResource->Map(0, nullptr, &mCpuVirtualAddress)))
    {
        mResource->Release();
        throw std::runtime_error("Failed to map constant buffer resource");
    }

    mGpuVirtualAddress = mResource->GetGPUVirtualAddress();

    // Initialize free lists
    mFreeLists.resize(mMaxOrder + 1);
    mFreeLists[mMaxOrder].push_back(0); // Entire pool is free initially
}

inline void D3D12ConstantBufferAllocator::MergeBuddies(size_t blockOffset, size_t order)
{
    if (order >= mMaxOrder)
        return;

    size_t buddyOffset = blockOffset ^ (1 << order);

    // Find buddy in free list
    auto& freeList = mFreeLists[order];
    auto buddyIt = std::find(freeList.begin(), freeList.end(), buddyOffset);

    if (buddyIt != freeList.end())
    {
        // Remove both blocks
        freeList.erase(std::remove(freeList.begin(), freeList.end(), blockOffset), freeList.end());
        freeList.erase(buddyIt);

        // Add merged block to higher order
        size_t parentOffset = std::min(blockOffset, buddyOffset);
        mFreeLists[order + 1].push_back(parentOffset);

        // Recursively merge
        MergeBuddies(parentOffset, order + 1);
    }
}

inline size_t D3D12ConstantBufferAllocator::GetOrder(size_t size)
{
    size_t roundedSize = NextPowerOfTwo(size);
    return static_cast<size_t>(log2(roundedSize));
}
