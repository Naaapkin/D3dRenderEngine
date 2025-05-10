#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/Render/PC/D3dUtil.h"
#include "Engine/Render/PC/Private/ResourceStateTracker.h"

class D3D12Resource
{
    friend struct std::hash<D3D12Resource>;
    
public:
    GUID Guid() const;
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
    uint32_t GetSubResourceCount() const;
    ID3D12Resource* D3D12ResourcePtr() const;
    void Detach();

    D3D12Resource();
    D3D12Resource(UComPtr<ID3D12Resource> pResource);
    ~D3D12Resource();

    DEFAULT_MOVE_CONSTRUCTOR(D3D12Resource);
    DEFAULT_MOVE_OPERATOR(D3D12Resource);
    NON_COPYABLE(D3D12Resource);

protected:
    UComPtr<ID3D12Resource> mResource;
};

template<>
struct std::hash<D3D12Resource>
{
    size_t operator()(const D3D12Resource& resource) const noexcept;
};

inline GUID D3D12Resource::Guid() const
{
    GUID guid;
    mResource->GetPrivateData(guid, nullptr, nullptr);
    return guid;
}

inline D3D12_GPU_VIRTUAL_ADDRESS D3D12Resource::GetGPUVirtualAddress() const
{
    return mResource->GetGPUVirtualAddress();
}

inline uint32_t D3D12Resource::GetSubResourceCount() const
{
    D3D12_RESOURCE_DESC desc = mResource->GetDesc();
    return static_cast<uint32_t>(desc.MipLevels) * desc.DepthOrArraySize;
}

inline ID3D12Resource* D3D12Resource::D3D12ResourcePtr() const
{
    return mResource.Get();
}

inline void D3D12Resource::Detach()
{
	mResource.Detach();
}

inline D3D12Resource::D3D12Resource() = default;

inline D3D12Resource::D3D12Resource(UComPtr<ID3D12Resource> pResource) : mResource(std::move(pResource)) { }

inline D3D12Resource::~D3D12Resource()
{
    ResourceStateTracker::RemoveResource(this);
    mResource.Release();
}

inline size_t std::hash<D3D12Resource>::operator()(const D3D12Resource& resource) const noexcept
{
    return reinterpret_cast<size_t>(resource.mResource.Get());
}
#endif
