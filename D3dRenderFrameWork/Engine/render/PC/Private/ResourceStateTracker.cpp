#ifdef WIN32
#include "ResourceStateTracker.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Resource/D3D12Resources.h"

StateConverter::~StateConverter()
{
    delete[] mFirstDstStates;
    mFirstDstStates = nullptr;
    delete[] mLastStates;
    mLastStates = nullptr;
}

StateConversion::StateConversion(): mIdx(0xffffffff), mSrcState(ResourceState::UNKNOWN), mDstState(ResourceState::UNKNOWN)
{}

StateConversion::StateConversion(uint32_t idx, ResourceState srcState, ResourceState dstState): mIdx(idx), mSrcState(srcState), mDstState(dstState)
{}

bool StateConversion::isGeneralConversion() const
{
    return mIdx == 0xffffffff;
}

// check if the conversion should be done instantly and explicitly.
// conversions: if all sub-resources are not in the same state: needed explicit conversions for sub-resources.
//              else: general conversion for total resource
std::vector<StateConversion> StateConverter::ConvertState(ResourceState dstState)
{
    std::vector<StateConversion> conversions;
    //mConverterState |= 0b100;   // make dirty
    if (!(mConverterState & 0b001))
    {
        conversions.resize(mNumSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < mNumSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.mIdx = i;
            conversion.mDstState = dstState;
            numExplicitConversions += ConvertSubResourceImpl(conversion);
        }
        TryFold();
        return std::move(conversions);
    }

    auto& srcState = mLastStates[0];
    // 1.mLastStates[0] != ResourceState::UNKNOWN: first conversion -- resolved on execution.
    // 2.ImplicitTransition: check if this conversion can be done implicitly -- we need to return if this conversion should be done explicitly, so negate it.
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::ImplicitTransition(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    if (isFirstConversion)
    {
        std::fill_n(mFirstDstStates, mNumSubResources, dstState);
    }
    else if (isExplicitConversion)
    {
        conversions.reserve(1);
        conversions.emplace_back(0xffffffff, srcState, dstState);
    }
    srcState = dstState;
    return std::move(conversions);
}

// returns whether the conversion should be done instantly and explicitly.
// conversion - conversion.second should be filled with the dst state.
//              conversion.first will get the src state if returns true. 
bool StateConverter::ConvertSubResource(StateConversion& conversion)
{
    // all initial states are same = isDirty, all destination states are same = false, make dirty;
    mConverterState = mConverterState >> 1 & 0b110 | 0b100; 
    return ConvertSubResourceImpl(conversion);
}

bool StateConverter::ConvertSubResourceImpl(StateConversion& conversion) const
{
    ResourceState& dstState = conversion.mDstState;
    ResourceState& srcState = mLastStates[conversion.mIdx];
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::ImplicitTransition(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
    mFirstDstStates[conversion.mIdx] = isFirstConversion ? dstState : mFirstDstStates[conversion.mIdx];
    const bool ret = !isFirstConversion && isExplicitConversion; 
    if (ret) conversion.mSrcState = srcState;       // fill the src state
    srcState = dstState;
    return ret;
}

std::vector<StateConversion> StateConverter::PreConvert(const ResourceState* srcStates) const
{
    std::vector<StateConversion> conversions(0);
    if (mConverterState & 0b010)
    {
        if (mFirstDstStates[0] == ResourceState::UNKNOWN) return conversions;
        StateConversion conversion{};
        conversion.mSrcState = srcStates[0];
        conversion.mDstState = mFirstDstStates[0];
        bool isExplicitConversion = !::ImplicitTransition(static_cast<uint32_t>(conversion.mSrcState),
                           *reinterpret_cast<uint32_t*>(&conversion.mDstState), mIsBufferOrSimultaneous);
        if (isExplicitConversion)
        {
            conversions.reserve(1);
            conversions.push_back(conversion);
        }
        return conversions;
    }
    conversions.reserve(mNumSubResources);
    uint32_t numExplicitConversions = 0;
    for (uint32_t i = 0; i < mNumSubResources; ++i)
    {
        if (mFirstDstStates[i] == ResourceState::UNKNOWN) continue;
        StateConversion conversion{i};
        conversion.mSrcState = srcStates[i];
        conversion.mDstState = mFirstDstStates[i];
        if (!::ImplicitTransition(static_cast<uint32_t>(conversion.mSrcState),
                                                     *reinterpret_cast<uint32_t*>(&conversion.mDstState),
                                                     mIsBufferOrSimultaneous))
        {
            conversions.push_back(conversion);
            numExplicitConversions++;
        }
    }
    return conversions;
}

std::vector<StateConversion> StateConverter::Join(const StateConverter& stateConverter)
{
    std::vector<StateConversion> conversions(0);

    // --------- folded to folded before conversion-----------
    // what we need to do is just add a total-resource-conversion between 'stateConverter' and this
    // if the conversion is explicit and not the first conversion.
    // ------------------other conditions---------------------
    // UnFold 'stateConverter'.
    // UnFold 'mFirstDstStates' if this resource is folded before conversion.
    // set 'conversions.capacity' to the count of sub-resources.
    // invoke stateConverter.ConvertSubResourceImpl() for every dst states in 'mFirstDstStates', count the explicit conversions and add the conversion to 'conversions'.
    // return conversions;

    if (stateConverter.mConverterState & 0b010 && mConverterState & 0b001)
    {
        if (mFirstDstStates[0] != ResourceState::UNKNOWN)
        {
            StateConversion conversion{};
            conversion.mSrcState = mLastStates[0];
            conversion.mDstState = stateConverter.mFirstDstStates[0];
            bool isExplicitConversion = !::ImplicitTransition(static_cast<uint32_t>(conversion.mSrcState),
                               *reinterpret_cast<uint32_t*>(&conversion.mDstState), mIsBufferOrSimultaneous);
            if (isExplicitConversion)
            {
                conversions.reserve(1);
                conversions.push_back(conversion);
            }
        }
    }
    else
    {
        mConverterState &= 0b110;   // subresources will be in different states after this transition
        conversions.resize(mNumSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < mNumSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.mIdx = i;
            conversion.mDstState = stateConverter.mFirstDstStates[i];
            numExplicitConversions += ConvertSubResourceImpl(conversion);
        }
    }
    memcpy(mLastStates, stateConverter.mLastStates, sizeof(ResourceState) * mNumSubResources);
    std::fill_n(stateConverter.mFirstDstStates, mNumSubResources, ResourceState::UNKNOWN);
    std::fill_n(stateConverter.mLastStates, mNumSubResources, ResourceState::UNKNOWN);
    return conversions;
}

const ResourceState* StateConverter::GetDesiredInitialState() const
{
    return mFirstDstStates;
}

const ResourceState* StateConverter::GetDestinationStates() const
{
    return mLastStates;
}

uint32_t StateConverter::GetSubResourceCount() const
{
    return mNumSubResources;
}

void StateConverter::Reset()
{
    std::fill_n(mFirstDstStates, mNumSubResources, ResourceState::UNKNOWN);
    std::fill_n(mLastStates, mNumSubResources, ResourceState::UNKNOWN);
    mConverterState = 0b011;
}

StateConverter::StateConverter() = default;

StateConverter::StateConverter(uint32_t numSubResources, bool isBufferOrSimultaneous) :
    mNumSubResources(numSubResources), mIsBufferOrSimultaneous(isBufferOrSimultaneous)
{
    mFirstDstStates = new ResourceState[mNumSubResources];
    mLastStates = new ResourceState[mNumSubResources];
    Reset();
}

StateConverter::StateConverter(StateConverter&& other) noexcept :
    mNumSubResources(other.mNumSubResources),
    mFirstDstStates(other.mFirstDstStates), mLastStates(other.mLastStates),
    mIsBufferOrSimultaneous(other.mIsBufferOrSimultaneous), mConverterState(other.mConverterState)
{
    other.mFirstDstStates = nullptr;
    other.mLastStates = nullptr;
}

StateConverter& StateConverter::operator=(StateConverter&& other) noexcept
{
    if (this != &other)
    {
        mNumSubResources = other.mNumSubResources;
        mFirstDstStates = other.mFirstDstStates;
        mLastStates = other.mLastStates;
        mIsBufferOrSimultaneous = other.mIsBufferOrSimultaneous;
        mConverterState = other.mConverterState;
        
        other.mFirstDstStates = nullptr;
        other.mLastStates = nullptr;
    }
    return *this;
}

void StateConverter::TryFold()
{
    if (mConverterState & 0b001) return;
    bool canFold = true;
    for (uint64_t i = 1; i < mNumSubResources; ++i)
    {
        canFold = mLastStates[i - 1] == mLastStates[i];
    }
    mConverterState = (mConverterState & 0b110) | canFold;
}

void ResourceStateTracker::AppendResource(const D3D12Resource* pResource, ResourceState initialState)
{
    sGlobalResourceStateMutex.lock();
    std::vector<ResourceState> states{ pResource->GetSubResourceCount() };
    std::fill_n(states.begin(), states.size(), initialState);
    sGlobalResourceStates.emplace(pResource, std::move(states));
    sGlobalResourceStateMutex.unlock();
}

void ResourceStateTracker::RemoveResource(const D3D12Resource* pResource)
{
    sGlobalResourceStateMutex.lock();
    sGlobalResourceStates.erase(pResource);
    sGlobalResourceStateMutex.unlock();
}

//void ResourceStateTracker::Track(const D3D12Resource& pResource)
//{
//    D3D12_RESOURCE_DESC&& desc = pResource.D3D12ResourcePtr()->GetDesc();
//    mStateConverters.try_emplace(&pResource, pResource.GetSubResourceCount(),
//        desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Flags &
//        D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
//}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::Join(ResourceStateTracker& other)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{64}; // TODO: 
    for (auto& pair : other.mStateConverters)
    {
        auto it = mStateConverters.find(pair.first);
        if (it == mStateConverters.end())
        {
            mStateConverters.emplace(pair.first, std::move(pair.second));
            continue;
        }
        StateConverter& converter = it->second;
        auto&& conversions = converter.Join(pair.second);
        if (conversions.empty()) continue;
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->D3D12ResourcePtr(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
            continue;
        }
        for (uint64_t i = 0; i < conversions.size(); ++i)
        {
            const auto& conversion = conversions[i];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->D3D12ResourcePtr(),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                                                               conversion.mIdx));
        }
    }
    Cancel();
    return barriers;
}

std::pair<bool, D3D12_RESOURCE_BARRIER> ResourceStateTracker::ConvertSubResourceState(
    const D3D12Resource* pResource, uint32_t subResourceIndex, ResourceState dstState)
{
    auto* converter = GetStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    StateConversion conversion{subResourceIndex, ResourceState::UNKNOWN, dstState};
    if (converter->ConvertSubResource(conversion))
    {
        return {true, CD3DX12_RESOURCE_BARRIER::Transition(pResource->D3D12ResourcePtr(),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            subResourceIndex)};
    }
    return {false, {}};
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::ConvertResourceState(const D3D12Resource* pResource, ResourceState dstState)
{
    StateConverter* converter = GetStateConverter(pResource);
    if (!converter) return {};
	std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    auto&& conversions = converter->ConvertState(dstState);
    if (conversions.empty()) return barriers;
    if (conversions[0].mIdx == 0xffffffff)
    {
        auto& conversion = conversions[0];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource->D3D12ResourcePtr(),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
        return barriers;
    }
    barriers.reserve(conversions.size());
    for (uint64_t i = 0; i < conversions.size(); ++i)
    {
        const auto& conversion = conversions[i];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource->D3D12ResourcePtr(),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            conversion.mIdx));
    }
    return barriers;
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::BuildPreTransitions() const
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    barriers.reserve(64);   // TODO: replace this magic-number(initial capacity)
    for (auto& pair : mStateConverters)
    {
        auto& converter = pair.second;
        sGlobalResourceStateMutex.lock_shared();
        auto&& conversions = converter.PreConvert(sGlobalResourceStates[pair.first].data());
        sGlobalResourceStateMutex.unlock_shared();
        if (conversions.empty()) continue;
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->D3D12ResourcePtr(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState)));
            continue;
        }
        for (const auto& conversion : conversions)
        {
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->D3D12ResourcePtr(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                conversion.mIdx));
        }
    }
    return barriers;
}

void ResourceStateTracker::StopTracking(bool isCopyQueue)
{
    sGlobalResourceStateMutex.lock();
    for (auto& pair : mStateConverters)
    {
        StateConverter& converter = pair.second;
        auto& currentStates = sGlobalResourceStates[pair.first];
        if (isCopyQueue)
        {
            std::fill_n(currentStates.data(), currentStates.size(), ResourceState::UNKNOWN);
        }
        else
        {
            const ResourceState* destinationStates = converter.GetDestinationStates();
            memcpy(currentStates.data(), destinationStates, sizeof(ResourceState) * currentStates.size());
        }
        converter.Reset();
    }
    mStateConverters.clear();
    sGlobalResourceStateMutex.unlock();
}

void ResourceStateTracker::Cancel()
{
    mStateConverters.clear();
}

StateConverter* ResourceStateTracker::GetStateConverter(const D3D12Resource* pResource)
{
    D3D12_RESOURCE_DESC&& desc = pResource->D3D12ResourcePtr()->GetDesc();
    const auto it = mStateConverters.try_emplace(pResource, pResource->GetSubResourceCount(),
        desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Flags &
        D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
    return &it.first->second;
}

std::unordered_map<const D3D12Resource*, std::vector<ResourceState>> ResourceStateTracker::sGlobalResourceStates{};
std::shared_mutex ResourceStateTracker::sGlobalResourceStateMutex{};
#endif