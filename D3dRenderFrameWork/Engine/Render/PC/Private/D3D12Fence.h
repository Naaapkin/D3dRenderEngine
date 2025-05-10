#pragma once
#ifdef WIN32
#include "Engine/Render/RHIDefination.h"
#include "Engine/Render/PC/Native/D3D12Device.h"

class D3D12Fence final : public RHIFence
{
public:
    uint64_t GetValue() const override;
    void Wait(uint64_t semaphore) const override;
    ID3D12Fence* GetD3D12Fence() const;
    D3D12Fence();
    D3D12Fence(UComPtr<ID3D12Fence> pFence);

private:
    UComPtr<ID3D12Fence> mFence;
};

inline uint64_t D3D12Fence::GetValue() const
{
    return mFence->GetCompletedValue();
}

inline void D3D12Fence::Wait(uint64_t semaphore) const
{
    if (mFence->GetCompletedValue() < semaphore)
    {
        const HANDLE he = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ThrowIfFailed(
            mFence->SetEventOnCompletion(semaphore, he)
        )
        WaitForSingleObject(he, INFINITE);
        CloseHandle(he);
    }
}

inline ID3D12Fence* D3D12Fence::GetD3D12Fence() const
{
    return mFence.Get();
}

inline D3D12Fence::D3D12Fence() = default;

inline D3D12Fence::D3D12Fence(UComPtr<ID3D12Fence> pFence) : RHIFence(), mFence(std::move(pFence))
{
}
#endif