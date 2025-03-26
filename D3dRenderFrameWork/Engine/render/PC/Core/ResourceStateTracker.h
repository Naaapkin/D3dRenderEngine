#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/D3dCommandListPool.h"
class D3dResource;
enum class ResourceState : uint32_t;

struct StateConversion
{
    bool isGeneralConversion() const
    {
        return idx == 0xffffffff;
    }
    
    uint64_t idx;
    ResourceState srcState;
    ResourceState dstState;
};

struct StateConverter
{
    bool isFoldedBeforeConversion() const;
    bool isCurrentFolded() const;
    std::vector<StateConversion> convert(ResourceState dstState);
    bool convertSub(uint64_t idx, StateConversion& conversion);
    std::vector<StateConversion> preConvert();
    std::vector<StateConversion> merge(StateConverter& stateConverter);
    void applyConvert(bool decayToCommon);
    StateConverter();
    StateConverter(D3dResource* pResource, bool mGeneralConversion = true);
    ~StateConverter();

    DEFAULT_MOVE_OPERATOR(StateConverter);
    DEFAULT_MOVE_CONSTRUCTOR(StateConverter);
    DELETE_COPY_OPERATOR(StateConverter);
    DELETE_COPY_CONSTRUCTOR(StateConverter);

private:
    bool convertSubImpl(StateConversion& conversion) const;
    void unfold();
    void tryFold();
    
    D3dResource* mResource;
    ResourceState* mFirstDstStates;
    ResourceState* mLastStates;
    bool mIsBufferOrSimultaneous;
    bool mFoldStateChanged;
    bool mIsFolded;
};

class ResourceStateTracker
{
public:
    void track(D3dResource& resource);
    std::vector<D3D12_RESOURCE_BARRIER> rightJoin(ResourceStateTracker& other);
    D3D12_RESOURCE_BARRIER convertSubResourceState(ID3D12Resource* pResource, uint64_t subResourceIndex, ResourceState dstState);
    std::vector<D3D12_RESOURCE_BARRIER> convertResourceState(ID3D12Resource* pResource, ResourceState dstState);
    std::vector<D3D12_RESOURCE_BARRIER> buildPreTransitions();
    std::unordered_map<ID3D12Resource*, StateConverter>& converters();
    void stopTracking();
    ResourceStateTracker();
    ~ResourceStateTracker();

    DELETE_COPY_OPERATOR(ResourceStateTracker);
    DELETE_COPY_CONSTRUCTOR(ResourceStateTracker);
    DEFAULT_MOVE_OPERATOR(ResourceStateTracker);
    DEFAULT_MOVE_CONSTRUCTOR(ResourceStateTracker);
    
private:
    StateConverter* getStateConverter(ID3D12Resource* pResource);
    
    std::unordered_map<ID3D12Resource*, StateConverter> mStateConverters;
};
#endif