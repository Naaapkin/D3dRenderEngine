#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/D3dObject.h"

class DescriptorHeap : public D3D12DeviceChild
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(uint32_t index) const;
    uint32_t descriptorSize() const;
    ID3D12DescriptorHeap* nativePtr() const override;

    DescriptorHeap(ID3D12DescriptorHeap* pDescHeap, uint32_t descriptorSize, uint32_t numDescriptors);
    ~DescriptorHeap() override = default;

    DELETE_COPY_CONSTRUCTOR(DescriptorHeap);
    DELETE_COPY_OPERATOR(DescriptorHeap);
    DEFAULT_MOVE_CONSTRUCTOR(DescriptorHeap);
    DEFAULT_MOVE_OPERATOR(DescriptorHeap);

private:
    uint32_t mNumDescriptors;
    uint32_t mDescriptorSize;
};

inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::cpuHandle(uint32_t index) const
{
    auto&& handle = nativePtr()->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mDescriptorSize * index);
    return handle;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::gpuHandle(uint32_t index) const
{
    auto&& handle = nativePtr()->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<UINT64>(mDescriptorSize * index);
    return handle;
}

inline uint32_t DescriptorHeap::descriptorSize() const
{
    return mDescriptorSize;
}

inline ID3D12DescriptorHeap* DescriptorHeap::nativePtr() const
{
    return static_cast<ID3D12DescriptorHeap*>(D3D12DeviceChild::nativePtr());
}

inline DescriptorHeap::DescriptorHeap(ID3D12DescriptorHeap* pDescHeap, uint32_t descriptorSize, uint32_t numDescriptors) :
    D3D12DeviceChild(pDescHeap),
    mNumDescriptors(numDescriptors),
    mDescriptorSize(descriptorSize)
{
}
#endif