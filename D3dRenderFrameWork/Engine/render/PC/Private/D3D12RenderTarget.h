#pragma once
#ifdef WIN32
#include "Engine/render/PC/Native/D3D12Resource.h"

class D3D12RenderTarget final : public RHIRenderTarget, D3D12Resource
{
    friend class D3D12SwapChain;
public:
    const D3D12Resource* GetD3D12Resource() const { return this; }
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle() const { return mRTHandle; }
    D3D12RenderTarget() = default;
    D3D12RenderTarget(const RHITextureDesc& desc, UComPtr<ID3D12Resource> pResource, const D3D12_CPU_DESCRIPTOR_HANDLE handle) : RHIRenderTarget(desc), D3D12Resource(std::move(pResource)), mRTHandle(handle) { }
    void Release() override { mResource.Release(); }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE mRTHandle;
};

class D3D12DepthStencil final : public RHIDepthStencil, D3D12Resource
{
public:
    const D3D12Resource* GetD3D12Resource() const { return this; }
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle() const { return mDSHandle; }
    void Release() override { mResource.Release(); }
    D3D12DepthStencil() = default;
    D3D12DepthStencil(const RHITextureDesc& desc, UComPtr<ID3D12Resource> pResource, const D3D12_CPU_DESCRIPTOR_HANDLE handle) : RHIDepthStencil(desc), D3D12Resource(std::move(pResource)), mDSHandle(handle) { }
    
private:
    D3D12_CPU_DESCRIPTOR_HANDLE mDSHandle;
};
#endif