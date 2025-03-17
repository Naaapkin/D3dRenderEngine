#pragma once

#ifdef WIN32
#include <pch.h>

class DescriptorHeap
{
public:
    ID3D12DescriptorHeap* HeapHandle() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(uint32_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(uint32_t index) const;
    uint32_t DescriptorSize() const;

    DescriptorHeap(ID3D12DescriptorHeap* pDescHeap, uint32_t descriptorSize);
    ~DescriptorHeap();

    DELETE_COPY_CONSTRUCTOR(DescriptorHeap);
    DELETE_COPY_OPERATOR(DescriptorHeap);
    DEFAULT_MOVE_CONSTRUCTOR(DescriptorHeap);
    DEFAULT_MOVE_OPERATOR(DescriptorHeap);

private:
    ComPtr<ID3D12DescriptorHeap> mDescHeap;
    uint32_t mDescriptorSize;
};
#endif