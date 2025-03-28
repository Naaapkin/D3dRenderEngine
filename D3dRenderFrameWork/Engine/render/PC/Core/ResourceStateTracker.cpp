#include "ResourceStateTracker.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Resource/D3dResource.h"

StateConverter::~StateConverter()
{
    delete[] mLastStates;
    delete[] mFirstDstStates;
}

StateConversion::StateConversion(): mIdx(0xffffffff), mSrcState(ResourceState::UNKNOWN), mDstState(ResourceState::UNKNOWN)
{}

StateConversion::StateConversion(uint64_t idx, ResourceState srcState, ResourceState dstState): mIdx(idx), mSrcState(srcState), mDstState(dstState)
{}

bool StateConversion::isGeneralConversion() const
{
    return mIdx == 0xffffffff;
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
    std::vector<StateConversion> conversions;
    uint64_t numSubResources = mResource->subResourceCount();
    if (!mIsFolded)
    {
        conversions.resize(numSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < numSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.mIdx = i;
            conversion.mDstState = dstState;
            numExplicitConversions += convertSubImpl(conversion);
        }
        tryFold();
        return std::move(conversions);
    }

    auto& srcState = mLastStates[0];
    // 1.mLastStates[0] != ResourceState::UNKNOWN: first conversion -- resolved on execution.
    // 2.gImplicitTransit: check if this conversion can be done implicitly -- we need to return if this conversion should be done explicitly, so negate it.
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    mFirstDstStates[0] = isFirstConversion ? dstState : mFirstDstStates[0];
    if (!isFirstConversion && isExplicitConversion)
    {
        conversions.reserve(1);
        conversions.emplace_back(StateConversion{0xffffffff, srcState, dstState});
    }
    srcState = dstState;
    return std::move(conversions);
}

// returns wether the conversion should be done instantly and explicitly.
// conversion - conversion.second should be filled with the dst state.
//              conversion.first will get the src state if returns true. 
bool StateConverter::convertSub(StateConversion& conversion)
{
    if (mIsFolded) unfold();
    return convertSubImpl(conversion);
}

bool StateConverter::convertSubImpl(StateConversion& conversion) const
{
    auto& dstState = conversion.mDstState;
    auto& srcState = mLastStates[conversion.mIdx];
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    mFirstDstStates[conversion.mIdx] = isFirstConversion ? dstState : mFirstDstStates[conversion.mIdx];
    const bool ret = !isFirstConversion && isExplicitConversion; 
    if (ret)
    {
        conversion.mSrcState = srcState;       // fill the src state
    }
    srcState = dstState;
    return ret;
}

std::vector<StateConversion> StateConverter::preConvert() const
{
    std::vector<StateConversion> conversions(0);
    if (isFoldedBeforeConversion())
    {
        if (mFirstDstStates[0] == ResourceState::UNKNOWN) return conversions;
        StateConversion conversion{};
        conversion.mSrcState = mResource->resourceStates()[0];
        conversion.mDstState = mFirstDstStates[0];
        bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(conversion.mSrcState),
                           *reinterpret_cast<uint32_t*>(&conversion.mDstState), mIsBufferOrSimultaneous);
        if (isExplicitConversion)
        {
            conversions.reserve(1);
            conversions.push_back(conversion);
        }
        return conversions;
    }
    uint64_t numSubResources = mResource->subResourceCount();
    conversions.reserve(numSubResources);
    uint64_t numExplicitConversions = 0;
    for (uint64_t i = 0; i < numSubResources; ++i)
    {
        if (mFirstDstStates[i] == ResourceState::UNKNOWN) continue;
        StateConversion conversion{i};
        conversion.mSrcState = mResource->resourceStates()[i];
        conversion.mDstState = mFirstDstStates[i];
        if (!::gImplicitTransit(static_cast<uint32_t>(conversion.mSrcState),
                                                     *reinterpret_cast<uint32_t*>(&conversion.mDstState),
                                                     mIsBufferOrSimultaneous))
        {
            conversions.push_back(conversion);
            numExplicitConversions++;
        }
    }
    return conversions;
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
            conversion.mIdx = i;
            conversion.mDstState = mFirstDstStates[i];
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
    mFirstDstStates = new ResourceState[numSubResources];
    std::fill_n(mFirstDstStates, numSubResources, ResourceState::UNKNOWN);
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
    mFirstDstStates = new ResourceState[numSubresources];
    mLastStates = new ResourceState[numSubresources];
    std::fill_n(mFirstDstStates, numSubresources, ResourceState::UNKNOWN);
    std::fill_n(mLastStates, numSubresources, ResourceState::UNKNOWN);
}

StateConverter::StateConverter(StateConverter&& other) noexcept :
    mResource(other.mResource),
    mFirstDstStates(other.mFirstDstStates), mLastStates(other.mLastStates),
    mIsBufferOrSimultaneous(other.mIsBufferOrSimultaneous), mFoldStateChanged(other.mFoldStateChanged), mIsFolded(other.mIsFolded)
{
}

StateConverter& StateConverter::operator=(StateConverter&& other) noexcept
{
    if (this !=&other )
    {
        mResource = other.mResource;
        mFirstDstStates = other.mFirstDstStates;
        mLastStates = other.mLastStates;
        mIsBufferOrSimultaneous = other.mIsBufferOrSimultaneous;
        mIsFolded = other.mIsFolded;
        mFoldStateChanged = other.mFoldStateChanged;

        other.mResource = nullptr;
        other.mFirstDstStates = nullptr;
        other.mLastStates = nullptr;
        other.mIsFolded = false;
        other.mIsBufferOrSimultaneous = false;
        other.mFoldStateChanged = false;
    }
    return *this;
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
    mStateConverters.try_emplace(resource.nativePtr(), &resource);
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
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
            continue;
        }
        for (uint64_t i = 0; i < conversions.size(); ++i)
        {
            const auto& conversion = conversions[i];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                                                               conversion.mIdx));
        }
    }
    other.cancel();
    return barriers;
}

D3D12_RESOURCE_BARRIER ResourceStateTracker::convertSubResourceState(ID3D12Resource* pResource, uint64_t subResourceIndex, ResourceState dstState)
{
    auto* converter = getStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    StateConversion conversion{subResourceIndex, ResourceState::UNKNOWN, dstState};
    if (converter->convertSub(conversion))
    {
        return CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
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
    if (conversions[0].mIdx == 0xffffffff)
    {
        auto& conversion = conversions[0];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
        return barriers;
    }
    barriers.reserve(conversions.size());
    for (uint64_t i = 0; i < conversions.size(); ++i)
    {
        const auto& conversion = conversions[i];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource,
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            conversion.mIdx));
    }
    return barriers;
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::buildPreTransitions() const
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    barriers.reserve(64);   // TODO: replace this magic-number(initial capacity)
    for (auto& pair : mStateConverters)
    {
        auto& converter = pair.second;
        auto&& conversions = std::move(converter.preConvert());
        if (conversions.empty()) continue;
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState)));
            continue;
        }
        for (const auto& conversion : conversions)
        {
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first,
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                conversion.mIdx));
        }
    }
    return barriers;
}

std::unordered_map<ID3D12Resource*, StateConverter>& ResourceStateTracker::converters()
{
    return mStateConverters;
}

void ResourceStateTracker::stopTracking(bool isCopyQueue)
{
    for (auto& pair : mStateConverters)
    {
        pair.second.applyConvert(isCopyQueue);
    }
    mStateConverters.clear();
}

void ResourceStateTracker::cancel()
{
    mStateConverters.clear();
}

StateConverter* ResourceStateTracker::getStateConverter(ID3D12Resource* pResource)
{
    const auto it = mStateConverters.find(pResource);
    if (it == mStateConverters.end()) return nullptr;
    return &it->second;
}

// void ResourceStateTracker::reset(D3dCommandListType mType)
// {
//     for (auto& converter : mStateConverters)
//     {
//         converter.second.applyConvert(mType == D3dCommandListType::COPY);
//     }
//     mStateConverters.clear();
// }

ResourceStateTracker::ResourceStateTracker() = default;

ResourceStateTracker::~ResourceStateTracker() = default;
