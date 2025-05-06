#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/helper.h"

class D3D12Buffer;
class D3D12Texture;
class D3D12Resource;

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

struct StateConversion
{
    StateConversion();
    explicit StateConversion(uint32_t idx, ResourceState srcState = ResourceState::UNKNOWN, ResourceState dstState = ResourceState::UNKNOWN);

    bool isGeneralConversion() const;

    uint32_t mIdx;
    ResourceState mSrcState;
    ResourceState mDstState;
};

struct StateConverter
{
    std::vector<StateConversion> ConvertState(ResourceState dstState);
    bool ConvertSubResource(StateConversion& conversion);
    std::vector<StateConversion> PreConvert(const ResourceState* srcStates) const;
    std::vector<StateConversion> Join(StateConverter& stateConverter);
    const ResourceState* GetDesiredInitialState() const;
    const ResourceState* GetDestinationStates() const;
    uint32_t GetSubResourceCount() const;
    void Reset();
    StateConverter();
    StateConverter(uint32_t numSubResources, bool isBufferOrSimultaneous);
    StateConverter(StateConverter&& other) noexcept;
    ~StateConverter();

    StateConverter& operator=(StateConverter&& other) noexcept;
    
    DELETE_COPY_OPERATOR(StateConverter);
    DELETE_COPY_CONSTRUCTOR(StateConverter);

private:
    bool ConvertSubResourceImpl(StateConversion& conversion) const;
    void TryFold();

    uint32_t mNumSubResources;
    ResourceState* mFirstDstStates;
    ResourceState* mLastStates;
    uint8_t mConverterState;    // Dirty | All initial states are same | All destination states are same 
    bool mIsBufferOrSimultaneous;
};

class ResourceStateTracker : NonCopyable
{
public:
    void Track(D3D12Buffer& buffer);
    void Track(D3D12Texture& texture);
    std::vector<D3D12_RESOURCE_BARRIER> rightJoin(ResourceStateTracker& other);
    std::pair<bool, D3D12_RESOURCE_BARRIER> ConvertSubResourceState(D3D12Resource* pResource, uint32_t subResourceIndex,
                                                                    ResourceState dstState);
    std::vector<D3D12_RESOURCE_BARRIER> ConvertResourceState(D3D12Resource* pResource, ResourceState dstState);
    std::vector<D3D12_RESOURCE_BARRIER> BuildPreTransitions() const;
    void StopTracking(bool isCopyQueue);
    void Cancel();
    
private:
    StateConverter* GetStateConverter(D3D12Resource* pResource);

    static std::unordered_map<D3D12Resource*, std::vector<ResourceState>, HashPtrAsTyped<D3D12Resource*>> sGlobalResourceStates;
    
    std::unordered_map<D3D12Resource*, StateConverter, HashPtrAsTyped<D3D12Resource*>> mStateConverters;
};
#endif