#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/D3dObject.h"

struct ResourceHandle
{
    uint64_t mIdx;
    uint8_t mNumDirt;
};

enum class ResourceState : uint32_t
{
    COMMON = D3D12_RESOURCE_STATE_COMMON,
    VERTEX_AND_CONSTANT_BUFFER = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
    INDEX_BUFFER = D3D12_RESOURCE_STATE_INDEX_BUFFER,
    RENDER_TARGET = D3D12_RESOURCE_STATE_RENDER_TARGET,
    UNORDERED_ACCESS = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    DEPTH_WRITE = D3D12_RESOURCE_STATE_DEPTH_WRITE,
    DEPTH_READ = D3D12_RESOURCE_STATE_DEPTH_READ,
    NON_PIXEL_SHADER_RESOURCE = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
    PIXEL_SHADER_RESOURCE = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
    STREAM_OUT = D3D12_RESOURCE_STATE_STREAM_OUT,
    INDIRECT_ARGUMENT = D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
    COPY_DEST = D3D12_RESOURCE_STATE_COPY_DEST,
    COPY_SOURCE = D3D12_RESOURCE_STATE_COPY_SOURCE,
    RESOLVE_DEST = D3D12_RESOURCE_STATE_RESOLVE_DEST,
    RESOLVE_SOURCE = D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
    READ = D3D12_RESOURCE_STATE_GENERIC_READ,
    PRESENT = D3D12_RESOURCE_STATE_PRESENT,
    PREDICATION = D3D12_RESOURCE_STATE_PREDICATION,
    VIDEO_DECODE_READ = D3D12_RESOURCE_STATE_VIDEO_DECODE_READ,
    VIDEO_DECODE_WRITE = D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE,
    VIDEO_PROCESS_READ = D3D12_RESOURCE_STATE_VIDEO_PROCESS_READ,
    VIDEO_PROCESS_WRITE = D3D12_RESOURCE_STATE_VIDEO_PROCESS_WRITE,
    VIDEO_ENCODE_READ = D3D12_RESOURCE_STATE_VIDEO_ENCODE_READ,
    VIDEO_ENCODE_WRITE = D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE,
    UNKNOWN = 0xffffffff,
};

class D3dResource : public D3dObject
{
    friend class D3dAllocator;
    
public:
    uint64_t subResourceCount() const;
    ResourceState* resourceStates() const;

    ID3D12Resource* nativePtr() const override;
    void release() override;
    ~D3dResource() override;
    
    D3dResource();
    D3dResource(D3dResource&& other) noexcept;
    D3dResource(ID3D12Resource* pResource, uint64_t subResourceCount, ResourceState initialState);

    DELETE_COPY_CONSTRUCTOR(D3dResource)
    DELETE_COPY_OPERATOR(D3dResource)

    bool operator==(const D3dResource& other) const noexcept;
    bool operator!=(const D3dResource& other) const noexcept;
    D3dResource& operator=(D3dResource&& other) noexcept;

private:
    uint64_t mNumSubResource;
    ResourceState* mResourceStates;
};

inline ID3D12Resource* D3dResource::nativePtr() const
{
    return reinterpret_cast<ID3D12Resource*>(D3dObject::nativePtr());
}

inline uint64_t D3dResource::subResourceCount() const
{
    return mNumSubResource;
}

inline ResourceState* D3dResource::resourceStates() const
{
    return mResourceStates;
}

inline void D3dResource::release()
{
    delete[] mResourceStates;
    mResourceStates = nullptr;
}

inline D3dResource::~D3dResource()
{
    D3dResource::release();
}

inline D3dResource::D3dResource() : D3dObject(nullptr), mNumSubResource(0), mResourceStates(nullptr) { }

inline D3dResource::D3dResource(D3dResource&& other) noexcept : D3dObject(std::move(other)), mNumSubResource(other.mNumSubResource), mResourceStates(other.mResourceStates)
{
    other.mResourceStates = nullptr;
    other.mNumSubResource = 0;
}

inline bool D3dResource::operator==(const D3dResource& other) const noexcept
{
    return D3dObject::operator==(other);
}

inline bool D3dResource::operator!=(const D3dResource& other) const noexcept
{
    return D3dObject::operator!=(other);
}

inline D3dResource& D3dResource::operator=(D3dResource&& other) noexcept
{
    if (&other != this)
    {
        D3dObject::operator=(std::move(other));
        mResourceStates = other.mResourceStates;
        mNumSubResource = other.mNumSubResource;
        other.mResourceStates = nullptr;
        other.mNumSubResource = 0;
    }
    return *this;
}

inline D3dResource::D3dResource(ID3D12Resource* pResource, 
                                uint64_t subResourceCount,
                                ResourceState initialState) :
        D3dObject(pResource),
        mNumSubResource(subResourceCount),
        mResourceStates(new ResourceState[subResourceCount])
{
    memset(mResourceStates, static_cast<int>(initialState), subResourceCount * sizeof(ResourceState));
}

#endif