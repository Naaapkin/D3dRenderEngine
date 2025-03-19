#ifdef WIN32
#include <Engine/render/PC/Core/DescriptorHeap.h>

ID3D12DescriptorHeap* DescriptorHeap::HeapHandle() const
{
    return mDescHeap.Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::CPUHandle(uint32_t index) const
{
    auto&& handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mDescriptorSize * index);
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GPUHandle(uint32_t index) const
{
    auto&& handle = mDescHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mDescriptorSize * index);
    return handle;
}

uint32_t DescriptorHeap::DescriptorSize() const
{
    return mDescriptorSize;
}

DescriptorHeap::DescriptorHeap(ID3D12DescriptorHeap* pDescHeap, uint32_t descriptorSize) :
    mDescHeap(pDescHeap),
    mDescriptorSize(descriptorSize) { }

DescriptorHeap::~DescriptorHeap() = default;
#endif