#pragma once
#include "Engine/render/PC/Native/D3D12Device.h"

class D3D12DescriptorHeap final : D3D12DeviceChild
{
public:
    GUID Guid() const override;
    void GrowIfNeeded(uint32_t numDescriptors);
    bool IsGPUVisible() const;
    D3D12_DESCRIPTOR_HEAP_DESC GetDesc() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(uint64_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(uint64_t index) const;
    ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const;
    uint64_t DescriptorSize() const;
    
    D3D12DescriptorHeap();
    D3D12DescriptorHeap(D3D12Device* parent, UComPtr<ID3D12DescriptorHeap> pHeap);

private:
    static constexpr uint64_t GROW_BLOCK_SIZE = 64;
    
    UComPtr<ID3D12DescriptorHeap> mDescHeap;
    uint64_t mDescSize = 0;
};

inline void D3D12DescriptorHeap::GrowIfNeeded(uint32_t numDescriptors)
{
    ID3D12Device* pDevice = Device()->GetD3D12Device();
    uint32_t cap = mDescHeap->GetDesc().NumDescriptors;
    if (cap < numDescriptors)
    {
        ID3D12DescriptorHeap* pNewHeap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = mDescHeap->GetDesc();
        desc.NumDescriptors = ::AlignUpToMul<uint64_t, GROW_BLOCK_SIZE>()(numDescriptors);
        pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pNewHeap));
        pDevice->CopyDescriptorsSimple(cap, pNewHeap->GetCPUDescriptorHandleForHeapStart(),
                                       mDescHeap->GetCPUDescriptorHandleForHeapStart(), desc.Type);
        mDescHeap.Release();
        mDescHeap.Reset(pNewHeap);
    }
}

inline bool D3D12DescriptorHeap::IsGPUVisible() const
{
    return mDescHeap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
}

inline GUID D3D12DescriptorHeap::Guid() const
{
    GUID guid;
    mDescHeap->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline D3D12_DESCRIPTOR_HEAP_DESC D3D12DescriptorHeap::GetDesc() const
{
    return mDescHeap->GetDesc();
}

inline D3D12_CPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::CPUHandle(uint64_t index) const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += index * mDescSize;
    return handle;
}

inline D3D12_GPU_DESCRIPTOR_HANDLE D3D12DescriptorHeap::GPUHandle(uint64_t index) const
{
    D3D12_GPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += index * mDescSize;
    return handle;
}

inline ID3D12DescriptorHeap* D3D12DescriptorHeap::GetD3D12DescriptorHeap() const
{
    return mDescHeap.Get();
}

inline uint64_t D3D12DescriptorHeap::DescriptorSize() const
{
    return mDescSize;
}

inline D3D12DescriptorHeap::D3D12DescriptorHeap() = default;

inline D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device* parent, UComPtr<ID3D12DescriptorHeap> pHeap): D3D12DeviceChild(parent),
                                                                                                           mDescHeap(std::move(pHeap)), mDescSize(Device()->GetD3D12Device()->GetDescriptorHandleIncrementSize(pHeap->GetDesc().Type))
{
}
