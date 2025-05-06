#pragma once
#include "Engine/render/PC/Resource/D3D12Resources.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "D3D12DescriptorHeap.h"

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
    D3D12DescriptorHeap* GetHeap() const;
    LinearDescriptorAllocator();

private:
    D3D12Device* mDevice;
    std::unique_ptr<D3D12DescriptorHeap> mHeap;
    
    uint32_t mCurrentOffset = 0;
    
    D3D12_CPU_DESCRIPTOR_HANDLE mCPUStart;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPUStart;
    
    std::mutex mMutex;
};

inline void LinearDescriptorAllocator::Initialize(D3D12Device* pDevice, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
    uint32_t descriptorCount, bool shaderVisible)
{
    mDevice = pDevice;
    mHeap.reset(new D3D12DescriptorHeap(pDevice, pDevice->CreateDescriptorHeap(
                                            heapType, shaderVisible
                                                          ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                                          : D3D12_DESCRIPTOR_HEAP_FLAG_NONE, descriptorCount)));

    mCPUStart = mHeap->CPUHandle(0);
    mGPUStart = shaderVisible ? 
                    mHeap->GPUHandle(0) : D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

inline D3D12DescriptorHandle LinearDescriptorAllocator::Allocate()
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCurrentOffset >= mHeap->GetDesc().NumDescriptors)
    {
        // 剩余空间不足，回绕到开头
        mCurrentOffset = 0;
    }

    D3D12DescriptorHandle descriptor {mHeap->CPUHandle(mCurrentOffset), mHeap->GPUHandle(mCurrentOffset)};
    mCurrentOffset++;
    return descriptor;
}

inline std::unique_ptr<D3D12DescriptorHandle[]> LinearDescriptorAllocator::Allocate(uint32_t count)
{
    std::lock_guard<std::mutex> lock(mMutex);
    if (mCurrentOffset + count > mHeap->GetDesc().NumDescriptors)
    {
        // 剩余空间不足，回绕到开头
        mCurrentOffset = 0;
    }
        
    std::unique_ptr<D3D12DescriptorHandle[]> descriptors{new D3D12DescriptorHandle[count]};
    D3D12DescriptorHandle base = {mHeap->CPUHandle(mCurrentOffset), mHeap->GPUHandle(mCurrentOffset)};
    bool isShaderVisible = mHeap->IsGPUVisible();
    uint64_t increment = mHeap->DescriptorSize();
    for (uint64_t i = 0; i < count; ++i)
    {
        descriptors[i] = base;
        base.mCPUHandle.ptr += increment;
        base.mGPUHandle.ptr += isShaderVisible ? increment : 0;
    }

    mCurrentOffset += count;
    return descriptors;
}

inline D3D12DescriptorHeap* LinearDescriptorAllocator::GetHeap() const
{ return mHeap.get(); }

inline LinearDescriptorAllocator::LinearDescriptorAllocator() = default;
#endif
