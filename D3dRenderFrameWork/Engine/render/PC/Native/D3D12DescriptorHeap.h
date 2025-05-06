#pragma once
#ifdef WIN32

class D3D12DescriptorHeap final
{
public:
    bool IsGPUVisible() const;
    D3D12_DESCRIPTOR_HEAP_DESC GetDesc() const;
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(uint64_t index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(uint64_t index) const;
    ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const;
    uint64_t DescriptorSize() const;
    
    D3D12DescriptorHeap();
    D3D12DescriptorHeap(UComPtr<ID3D12DescriptorHeap>&& pHeap, uint32_t descSize);

private:
    UComPtr<ID3D12DescriptorHeap> mDescHeap;
    uint64_t mDescSize = 0;
};

inline bool D3D12DescriptorHeap::IsGPUVisible() const
{
    return mDescHeap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
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

inline D3D12DescriptorHeap::D3D12DescriptorHeap(UComPtr<ID3D12DescriptorHeap>&& pHeap, uint32_t descSize) : mDescHeap(std::move(pHeap)), mDescSize(descSize)
{
}
#endif