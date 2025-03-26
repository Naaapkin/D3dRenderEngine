#include "ResourceStateTracker.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Resource/D3dResource.h"

StateConverter::~StateConverter()
{
    delete[] mLastStates;
    delete[] mFirstDstStates;
}

bool StateConverter::isFoldedBeforeConversion() const
{
    return mIsFolded ^ mFoldStateChanged;
}

bool StateConverter::isCurrentFolded() const
{
    return mIsFolded;
}

// check if the conversion should be done instantly and explicitly.
// conversions: if all sub-resources are not in the same state: needed explicit conversions for sub-resources.
//              else: general conversion for total resource
std::vector<StateConversion> StateConverter::convert(ResourceState dstState)
{
    std::vector<StateConversion> conversions{0};
    uint64_t numSubResources = mResource->subResourceCount();
    if (!mIsFolded)
    {
        conversions.resize(numSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < numSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.idx = i;
            conversion.dstState = dstState;
            numExplicitConversions += convertSubImpl(conversion);
        }
        tryFold();
        return std::move(conversions);
    }

    auto& srcState = mLastStates[0];
    // 1.mLastStates[0] != ResourceState::UNKNOWN: first conversion -- resolved on execution.
    // 2.gImplicitTransit: check if this conversion can be done implicitly -- we need to return if this conversion should be done explicitly, so negate it.
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = ::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    mFirstDstStates[0] = isFirstConversion ? dstState : mFirstDstStates[0];
    if (!isFirstConversion && isExplicitConversion)
    {
        conversions.reserve(1);
        conversions.emplace_back(StateConversion{0xffffffff, srcState, dstState});
    }
    return std::move(conversions);
}

// returns wether the conversion should be done instantly and explicitly.
// conversion - conversion.second should be filled with the dst state.
//              conversion.first will get the src state if returns true. 
bool StateConverter::convertSub(uint64_t idx, StateConversion& conversion)
{
    if (mIsFolded) unfold();
    return convertSubImpl(conversion);
}

bool StateConverter::convertSubImpl(StateConversion& conversion) const
{
    auto& dstState = conversion.dstState;
    auto& srcState = mLastStates[conversion.idx];
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = ::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    mFirstDstStates[conversion.idx] = isFirstConversion ? dstState : mFirstDstStates[conversion.idx];
    const bool ret = !isFirstConversion && isExplicitConversion; 
    if (ret)
    {
        conversion.srcState = srcState;       // fill the src state
        srcState = dstState;               // update current state
    }
    return ret;
}

std::vector<StateConversion> StateConverter::preConvert()
{
    std::vector<StateConversion> conversions(0);
    if (isFoldedBeforeConversion() && mFirstDstStates[0] != ResourceState::UNKNOWN)
    {
        return std::move(convert(mFirstDstStates[0]));
    }
    uint64_t numSubResources = mResource->subResourceCount();
    conversions.resize(numSubResources);
    uint64_t numExplicitConversions = 0;
    for (uint64_t i = 0; i < numSubResources; ++i)
    {
        auto& conversion = conversions[numExplicitConversions];
        conversion.idx = i;
        conversion.dstState = mFirstDstStates[i];
        numExplicitConversions += mFirstDstStates[i] != ResourceState::UNKNOWN && convertSubImpl(conversion);
    }
    return std::move(conversions);
}

std::vector<StateConversion> StateConverter::merge(StateConverter& stateConverter)
{
    std::vector<StateConversion> conversions(0);

    // --------- folded to folded before conversion-----------
    // what we need to do is just add a total-resource-conversion between 'stateConverter' and this.
    // and add the conversion if it's explicit and is not the first conversion.
    // ------------------other conditions---------------------
    // unfold 'stateConverter'.
    // unfold 'mFirstDstStates' if this resource is folded before conversion.
    // set 'conversions.capacity' to the count of sub-resources.
    // invoke stateConverter.convertSubImpl() for every dst states in 'mFirstDstStates', count the explicit conversions and add the conversion to 'conversions'.
    // return conversions;

    if (isFoldedBeforeConversion() && stateConverter.isCurrentFolded())
    {
        if (mFirstDstStates[0] != ResourceState::UNKNOWN)
        {
            conversions = std::move(stateConverter.convert(mFirstDstStates[0]));
        }
    }
    else
    {
        stateConverter.unfold();
        uint64_t numSubResources = mResource->subResourceCount();
        if (isFoldedBeforeConversion())
        {
            auto& buffer = mFirstDstStates[0];
            delete[] mFirstDstStates;
            mFirstDstStates = new ResourceState[numSubResources];
            std::fill_n(mFirstDstStates, numSubResources, buffer);
        }
        conversions.resize(numSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < numSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.idx = i;
            conversion.dstState = mFirstDstStates[i];
            numExplicitConversions += mFirstDstStates[i] != ResourceState::UNKNOWN && convertSubImpl(conversion);
        }
    }
    delete[] mFirstDstStates;
    delete[] stateConverter.mLastStates;
    mFirstDstStates = stateConverter.mFirstDstStates;
    stateConverter.mFirstDstStates = nullptr;
    stateConverter.mLastStates = nullptr;
    return conversions;
}

void StateConverter::applyConvert(bool decayToCommon)
{
    uint64_t numSubResources = mResource->subResourceCount();
    uint64_t num = mIsFolded ? 1 : numSubResources;
    decayToCommon = decayToCommon || mIsBufferOrSimultaneous;
    auto* resourceStates = mResource->resourceStates();
    if (mIsFolded || decayToCommon)
    {
        std::fill_n(resourceStates, numSubResources, decayToCommon ? ResourceState::COMMON : mLastStates[0]);
    }
    else
    {
        memcpy(resourceStates, mLastStates, sizeof(ResourceState) * numSubResources);
    }

    delete[] mFirstDstStates;
    mFirstDstStates = new ResourceState[num];
    std::fill_n(mFirstDstStates, num, ResourceState::UNKNOWN);
    mFoldStateChanged = false;
}

StateConverter::StateConverter() = default;

StateConverter::StateConverter(D3dResource* pResource, bool mGeneralConversion) :
    mResource(pResource),
    mFoldStateChanged(false),
    mIsFolded(mGeneralConversion)
{
    auto desc = mResource->nativePtr()->GetDesc();
    mIsBufferOrSimultaneous = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    uint64_t numSubresources = mResource->subResourceCount();
    uint64_t num = mGeneralConversion ? 1 : numSubresources;
    mFirstDstStates = new ResourceState[num];
    mLastStates = new ResourceState[num];
    std::fill_n(mFirstDstStates, num, ResourceState::UNKNOWN);
    memcpy(mResource->resourceStates(), mFirstDstStates, numSubresources * sizeof(ResourceState));
}

void StateConverter::unfold()
{
    if (!mIsFolded) return;
    mFoldStateChanged = true;
    uint64_t numSubResources = mResource->subResourceCount();
    ResourceState buffer = mLastStates[0];
    delete[] mLastStates;
    mLastStates = new ResourceState[numSubResources];
    std::fill_n(mLastStates, numSubResources, buffer);
    buffer = mFirstDstStates[0];
    delete[] mFirstDstStates;
    mFirstDstStates = new ResourceState[numSubResources];
    std::fill_n(mFirstDstStates, numSubResources, buffer);
}

void StateConverter::tryFold()
{
    if (mIsFolded) return;
    uint64_t numSubResources = mResource->subResourceCount();
    bool canFold = true;
    for (uint64_t i = 1; i < numSubResources; ++i)
    {
        canFold = mLastStates[i - 1] == mLastStates[i];
    }
    mIsFolded = canFold;
    if (canFold)
    {
        mFoldStateChanged = true;
        // don't resize 'mFirstDstStates', it's needed for resolving the first conversions.
        ResourceState buffer = mLastStates[0];
        delete[] mLastStates;
        mLastStates = new ResourceState[] { buffer };
    }
}

void ResourceStateTracker::track(D3dResource& resource)
{
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(resource.nativePtr(), TEXT("resource is invalid"));
#endif
    mStateConverters.try_emplace(resource.nativePtr(), StateConverter{ &resource });
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::rightJoin(ResourceStateTracker& other)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{64}; // TODO: 
    for (auto& pair : other.mStateConverters)
    {
        StateConverter* pConverter = getStateConverter(pair.first);
        if (!pConverter)
        {
            mStateConverters.emplace(pair.first, std::move(pair.second));
            continue;
        }
        auto&& conversions = pair.second.merge(*pConverter);
        if (conversions.empty()) continue;
        if (conversions[0].idx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
            continue;
        }
        for (uint64_t i = 0; i < conversions.size(); ++i)
        {
            const auto& conversion = conversions[i];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
                                                               conversion.idx));
        }
    }
    other.stopTracking();
    return barriers;
}

D3D12_RESOURCE_BARRIER ResourceStateTracker::convertSubResourceState(ID3D12Resource* pResource, uint64_t subResourceIndex, ResourceState dstState)
{
    auto* converter = getStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    StateConversion conversion{subResourceIndex, ResourceState::UNKNOWN, dstState};
    if (converter->convertSub(subResourceIndex, conversion))
    {
        return CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
            subResourceIndex);
    }
    return {};
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::convertResourceState(ID3D12Resource* pResource, ResourceState dstState)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    auto* converter = getStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    auto&& conversions = converter->convert(dstState);
    if (conversions.empty()) return barriers;
    if (conversions[0].idx == 0xffffffff)
    {
        auto& conversion = conversions[0];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
        return barriers;
    }
    barriers.reserve(conversions.size());
    for (uint64_t i = 0; i < conversions.size(); ++i)
    {
        const auto& conversion = conversions[i];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
            conversion.idx));
    }
    return barriers;
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::buildPreTransitions()
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    barriers.reserve(64);   // TODO: replace this magic-number(initial capacity)
    for (auto& pair : mStateConverters)
    {
        auto& converter = pair.second;
        auto&& conversions = std::move(converter.preConvert());
        if (conversions.empty()) continue;
        if (conversions[0].idx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.dstState)));
            continue;
        }
        for (const auto& conversion : conversions)
        {
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.srcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.dstState),
                conversion.idx));
        }
    }
    return barriers;
}

std::unordered_map<ID3D12Resource*, StateConverter>& ResourceStateTracker::converters()
{
    return mStateConverters;
}

void ResourceStateTracker::stopTracking()
{
    mStateConverters.clear();
}

StateConverter* ResourceStateTracker::getStateConverter(ID3D12Resource* pResource)
{
    const auto it = mStateConverters.find(pResource);
    if (it == mStateConverters.end()) return nullptr;
    return &it->second;
}

// void ResourceStateTracker::reset(D3dCommandListType type)
// {
//     for (auto& converter : mStateConverters)
//     {
//         converter.second.applyConvert(type == D3dCommandListType::COPY);
//     }
//     mStateConverters.clear();
// }

ResourceStateTracker::ResourceStateTracker() = default;

ResourceStateTracker::~ResourceStateTracker() = default;
