#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/Memory/LinearAllocator.h"
#include "Engine/Memory/BlockAllocator.h"
#include "Engine/Memory/RingAllocator.h"
#include "Engine/Render/PC/Native/D3D12DescriptorHeap.h"

struct D3D12DescriptorHandle
{
    D3D12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
};

class LinearDescriptorAllocator
{
public:
    void Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount, bool shaderVisible = false);
    // 分配单个描述符
    D3D12DescriptorHandle Allocate();
    // 分配连续多个描述符
    std::unique_ptr<D3D12DescriptorHandle[]> Allocate(uint32_t count);
    void Reset();
    D3D12DescriptorHeap* GetHeap() const;
    LinearDescriptorAllocator();

private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12DescriptorHeap> mHeap;
    LinearAllocator mAllocator;
    
    D3D12_CPU_DESCRIPTOR_HANDLE mCPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUStart;
    
    std::mutex mMutex;
};

class RingDescriptorAllocator
{
public:
    void Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount, bool shaderVisible = false);
    // 分配单个描述符
    D3D12DescriptorHandle Allocate();
    // 分配连续多个描述符
    std::unique_ptr<D3D12DescriptorHandle[]> Allocate(uint32_t count);
    void Reset();
    D3D12DescriptorHeap* GetHeap() const;
    RingDescriptorAllocator();

private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12DescriptorHeap> mHeap;
    UnsafeRingAllocator mAllocator;
    
    D3D12_CPU_DESCRIPTOR_HANDLE mCPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUStart;
    
    std::mutex mMutex;
};

class BlockDescriptorAllocator
{
public:
    void Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t descriptorCount, bool shaderVisible = false);
    std::unique_ptr<D3D12DescriptorHandle[]> Allocate(uint64_t size);
    D3D12DescriptorHandle Allocate();
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, bool& succeeded);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint64_t size, bool& succeeded);
    void Free(D3D12_GPU_DESCRIPTOR_HANDLE handle, bool& succeeded);
    void Free(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint64_t size, bool& succeeded);

    BlockDescriptorAllocator();
    
private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12DescriptorHeap> mHeap;
    BlockAllocator mAllocator;

    D3D12_CPU_DESCRIPTOR_HANDLE mCPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUStart;
    
    std::mutex mMutex;
};

inline void LinearDescriptorAllocator::Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    uint32_t descriptorCount, bool shaderVisible)
{
    mDevice = pDevice;
    mHeap.reset(new D3D12DescriptorHeap(pDevice->CreateDescriptorHeap(
        heapType, shaderVisible
                      ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                      : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, descriptorCount),
        pDevice->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType)));
    mAllocator.Initialize(descriptorCount);

    mCPUStart = mHeap->CPUHandle(0);
    mGPUStart = shaderVisible ? mHeap->GPUHandle(0) : D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

inline D3D12DescriptorHandle LinearDescriptorAllocator::Allocate()
{
    std::lock_guard<std::mutex> lock(mMutex);
    const uint64_t offset = mAllocator.Allocate();
    if (offset == MAXUINT64) return {MAXUINT64, MAXUINT64};
    
    return {mHeap->CPUHandle(offset), mHeap->GPUHandle(offset)};
}

inline std::unique_ptr<D3D12DescriptorHandle[]> LinearDescriptorAllocator::Allocate(uint32_t count)
{
    std::lock_guard<std::mutex> lock(mMutex);
    const uint64_t offset = mAllocator.Allocate(count);
        
    std::unique_ptr<D3D12DescriptorHandle[]> descriptors{new D3D12DescriptorHandle[count]};
    bool isShaderVisible = mHeap->IsGPUVisible();
    uint64_t increment = mHeap->DescriptorSize();
    D3D12DescriptorHandle base = {mHeap->CPUHandle(offset), isShaderVisible ? D3D12_GPU_DESCRIPTOR_HANDLE{0} : mHeap->GPUHandle(offset)};
    for (uint64_t i = 0; i < count; ++i)
    {
        descriptors[i] = base;
        base.mCPUHandle.ptr += increment;
        base.mGPUHandle.ptr += isShaderVisible ? increment : 0;
    }
    return descriptors;
}

inline void LinearDescriptorAllocator::Reset()
{
    mAllocator.Reset();
}

inline D3D12DescriptorHeap* LinearDescriptorAllocator::GetHeap() const
{ return mHeap.get(); }

inline LinearDescriptorAllocator::LinearDescriptorAllocator() = default;

inline void RingDescriptorAllocator::Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    uint32_t descriptorCount, bool shaderVisible)
{
    mDevice = pDevice;
    mHeap.reset(new D3D12DescriptorHeap(pDevice->CreateDescriptorHeap(
        heapType, shaderVisible
                      ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                      : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, descriptorCount),
        pDevice->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType)));
    mAllocator.Initialize(descriptorCount);

    mCPUStart = mHeap->CPUHandle(0);
    mGPUStart = shaderVisible ? mHeap->GPUHandle(0) : D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

inline D3D12DescriptorHandle RingDescriptorAllocator::Allocate()
{
    std::lock_guard<std::mutex> lock(mMutex);
    const uint64_t offset = mAllocator.Allocate(1);
    if (offset == MAXUINT64) return {MAXUINT64, MAXUINT64};
    return {mHeap->CPUHandle(offset), mHeap->GPUHandle(offset)};
}

inline std::unique_ptr<D3D12DescriptorHandle[]> RingDescriptorAllocator::Allocate(uint32_t count)
{
    std::lock_guard<std::mutex> lock(mMutex);
    uint64_t offset = mAllocator.Allocate(count);
        
    std::unique_ptr<D3D12DescriptorHandle[]> descriptors{new D3D12DescriptorHandle[count]};
    uint64_t increment = mHeap->DescriptorSize();
    D3D12DescriptorHandle base = {mHeap->CPUHandle(offset), mHeap->GPUHandle(offset)};
    for (uint64_t i = 0; i < count; ++i)
    {
        descriptors[i] = base;
        
        if (offset >= mAllocator.GetTotalSize())
        {
            base = {mHeap->CPUHandle(0), mHeap->GPUHandle(0)};
            offset = 0;
        }
        else
        {
            base.mCPUHandle.ptr += increment;
            base.mGPUHandle.ptr += increment;
            offset++;
        }
    }
    return descriptors;
}

inline void RingDescriptorAllocator::Reset()
{
    std::lock_guard<std::mutex> lock(mMutex);
    mAllocator.Reset();
}

inline D3D12DescriptorHeap* RingDescriptorAllocator::GetHeap() const
{
    return mHeap.get();
}

inline RingDescriptorAllocator::RingDescriptorAllocator() = default;

inline void BlockDescriptorAllocator::Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
                                                 uint32_t descriptorCount, bool shaderVisible)
{
    mDevice = pDevice;
    mHeap.reset(new D3D12DescriptorHeap(pDevice->CreateDescriptorHeap(
                                            heapType, shaderVisible
                                                          ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                                          : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, descriptorCount),
                                        pDevice->GetD3D12Device()->GetDescriptorHandleIncrementSize(heapType)));
    mAllocator.Initialize(descriptorCount);

    mCPUStart = mHeap->CPUHandle(0);
    mGPUStart = shaderVisible ? mHeap->GPUHandle(0) : D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

inline std::unique_ptr<D3D12DescriptorHandle[]> BlockDescriptorAllocator::Allocate(uint64_t size)
{
    std::unique_ptr<uint64_t[]>&& offsets = mAllocator.Allocate(size);
    if (!offsets) return nullptr;
    std::unique_ptr<D3D12DescriptorHandle[]> descriptors{new D3D12DescriptorHandle[size]};
    bool isShaderVisible = mHeap->IsGPUVisible();
    for (uint64_t i = 0; i < size; ++i)
    {
        descriptors[i].mCPUHandle = mHeap->CPUHandle(offsets[i]);
        if (isShaderVisible) descriptors[i].mGPUHandle = mHeap->GPUHandle(offsets[i]);
    }
    return std::move(descriptors);
}

inline D3D12DescriptorHandle BlockDescriptorAllocator::Allocate()
{
    uint64_t offset = mAllocator.Allocate();
    if (offset == MAXUINT64) return { {MAXUINT64}, {MAXUINT64} };
    return { { mHeap->CPUHandle(offset) }, { mHeap->IsGPUVisible() ? mHeap->GPUHandle(offset) : D3D12_GPU_DESCRIPTOR_HANDLE{ MAXUINT64 } } };
}

inline void BlockDescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, bool& succeeded)
{
    const uint64_t offset = (handle.ptr - mCPUStart.ptr) / mHeap->DescriptorSize();
    succeeded = mAllocator.Free(offset);
}

inline void BlockDescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint64_t size, bool& succeeded)
{
    const uint64_t offset = (handle.ptr - mCPUStart.ptr) / mHeap->DescriptorSize();
    for (uint64_t i = 0; i < size; ++i)
    {
        succeeded = succeeded && mAllocator.Free(offset + i);
    }
}

inline void BlockDescriptorAllocator::Free(D3D12_GPU_DESCRIPTOR_HANDLE handle, bool& succeeded)
{
    const uint64_t offset = handle.ptr - mGPUStart.ptr;
    succeeded = mAllocator.Free(offset);
}

inline void BlockDescriptorAllocator::Free(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint64_t size, bool& succeeded)
{
    const uint64_t offset = handle.ptr - mGPUStart.ptr;
    for (uint64_t i = 0; i < size; ++i)
    {
        succeeded = succeeded && mAllocator.Free(offset + i);
    }
}

inline BlockDescriptorAllocator::BlockDescriptorAllocator() = default;
#endif
