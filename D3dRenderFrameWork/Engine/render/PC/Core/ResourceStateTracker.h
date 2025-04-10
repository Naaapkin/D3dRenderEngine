#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/PC/Resource/D3D12Resources.h"

class D3D12Resource;
enum class ResourceState : uint32_t;

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
    D3D12_RESOURCE_BARRIER ConvertSubResourceState(D3D12Resource* pResource, uint32_t subResourceIndex, ResourceState dstState);
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