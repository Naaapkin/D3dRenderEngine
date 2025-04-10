#include "ResourceStateTracker.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Resource/D3D12Resources.h"
#include "Engine/render/PC/Resource/D3dResource.h"

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
    mConverterState |= 0b100;   // make dirty
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
    // 2.gImplicitTransit: check if this conversion can be done implicitly -- we need to return if this conversion should be done explicitly, so negate it.
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
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
    // all initial states are same = dirty, all destination states are same = false, make dirty;
    mConverterState = mConverterState >> 1 & 0b110 | 0b100; 
    return ConvertSubResourceImpl(conversion);
}

bool StateConverter::ConvertSubResourceImpl(StateConversion& conversion) const
{
    auto& dstState = conversion.mDstState;
    auto& srcState = mLastStates[conversion.mIdx];
    bool isFirstConversion = srcState == ResourceState::UNKNOWN;
    bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(srcState), *reinterpret_cast<uint32_t*>(&dstState), mIsBufferOrSimultaneous);
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
        bool isExplicitConversion = !::gImplicitTransit(static_cast<uint32_t>(conversion.mSrcState),
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

std::vector<StateConversion> StateConverter::Join(StateConverter& stateConverter)
{
    std::vector<StateConversion> conversions(0);

    // --------- folded to folded before conversion-----------
    // what we need to do is just add a total-resource-conversion between 'stateConverter' and this.
    // and add the conversion if it's explicit and is not the first conversion.
    // ------------------other conditions---------------------
    // UnFold 'stateConverter'.
    // UnFold 'mFirstDstStates' if this resource is folded before conversion.
    // set 'conversions.capacity' to the count of sub-resources.
    // invoke stateConverter.ConvertSubResourceImpl() for every dst states in 'mFirstDstStates', count the explicit conversions and add the conversion to 'conversions'.
    // return conversions;

    if (mConverterState & 0b010 && stateConverter.mConverterState & 0b001)
    {
        if (mFirstDstStates[0] != ResourceState::UNKNOWN)
        {
            StateConversion conversion{};
            conversion.mSrcState = stateConverter.mLastStates[0];
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
    }
    else
    {
        stateConverter.mConverterState &= 0b110;
        conversions.resize(mNumSubResources);
        uint64_t numExplicitConversions = 0;
        for (uint64_t i = 0; i < mNumSubResources; ++i)
        {
            auto& conversion = conversions[numExplicitConversions];
            conversion.mIdx = i;
            conversion.mDstState = mFirstDstStates[i];
            numExplicitConversions += mFirstDstStates[i] != ResourceState::UNKNOWN && ConvertSubResourceImpl(conversion);
        }
    }
    delete[] mFirstDstStates;
    delete[] stateConverter.mLastStates;
    mFirstDstStates = stateConverter.mFirstDstStates;
    stateConverter.mFirstDstStates = nullptr;
    stateConverter.mLastStates = nullptr;
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
    mConverterState = 0b001;
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
    mConverterState |= canFold;
}

void ResourceStateTracker::Track(D3D12Buffer& buffer)
{
    mStateConverters.try_emplace(&buffer, StateConverter{buffer.GetSubResourceCount(), true});
}

void ResourceStateTracker::Track(D3D12Texture& texture)
{
    mStateConverters.try_emplace(&texture, StateConverter{texture.GetSubResourceCount(), false});
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::rightJoin(ResourceStateTracker& other)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{64}; // TODO: 
    for (auto& pair : other.mStateConverters)
    {
        StateConverter* pConverter = GetStateConverter(pair.first);
        if (!pConverter)
        {
            mStateConverters.emplace(pair.first, std::move(pair.second));
            continue;
        }
        auto&& conversions = pair.second.Join(*pConverter);
        if (conversions.empty()) continue;
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->NativeResource(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
            continue;
        }
        for (uint64_t i = 0; i < conversions.size(); ++i)
        {
            const auto& conversion = conversions[i];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->NativeResource(),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                                                               static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                                                               conversion.mIdx));
        }
    }
    other.Cancel();
    return barriers;
}

D3D12_RESOURCE_BARRIER ResourceStateTracker::ConvertSubResourceState(D3D12Resource* pResource, uint32_t subResourceIndex, ResourceState dstState)
{
    auto* converter = GetStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    StateConversion conversion{subResourceIndex, ResourceState::UNKNOWN, dstState};
    if (converter->ConvertSubResource(conversion))
    {
        return CD3DX12_RESOURCE_BARRIER::Transition(pResource->NativeResource(),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            subResourceIndex);
    }
    return {};
}

std::vector<D3D12_RESOURCE_BARRIER> ResourceStateTracker::ConvertResourceState(D3D12Resource* pResource, ResourceState dstState)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers{};
    auto* converter = GetStateConverter(pResource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(converter, TEXT("resource not declared as used\n"));
#endif
    auto&& conversions = converter->ConvertState(dstState);
    if (conversions.empty()) return barriers;
    if (conversions[0].mIdx == 0xffffffff)
    {
        auto& conversion = conversions[0];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource->NativeResource(),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
            static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES));
        return barriers;
    }
    barriers.reserve(conversions.size());
    for (uint64_t i = 0; i < conversions.size(); ++i)
    {
        const auto& conversion = conversions[i];
        barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pResource->NativeResource(),
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
        auto&& conversions = std::move(converter.PreConvert(sGlobalResourceStates[pair.first].data()));
        if (conversions.empty()) continue;
        if (conversions[0].mIdx == 0xffffffff)
        {
            auto& conversion = conversions[0];
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->NativeResource(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState)));
            continue;
        }
        for (const auto& conversion : conversions)
        {
            barriers.emplace_back(CD3DX12_RESOURCE_BARRIER::Transition(pair.first->NativeResource(),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mSrcState),
                static_cast<D3D12_RESOURCE_STATES>(conversion.mDstState),
                conversion.mIdx));
        }
    }
    return barriers;
}

void ResourceStateTracker::StopTracking(bool isCopyQueue)
{
    for (auto& pair : mStateConverters)
    {
        StateConverter& converter = pair.second;
        auto& currentStates = sGlobalResourceStates[pair.first];
        if (isCopyQueue)
        {
            std::fill_n(currentStates.begin(), currentStates.size(), ResourceState::UNKNOWN);
        }
        else
        {
            const ResourceState* destinationStates = converter.GetDestinationStates();
            memcpy(currentStates.data(), destinationStates, sizeof(ResourceState) * currentStates.size());
        }
        converter.Reset();
    }
    mStateConverters.clear();
}

void ResourceStateTracker::Cancel()
{
    mStateConverters.clear();
}

StateConverter* ResourceStateTracker::GetStateConverter(D3D12Resource* pResource)
{
    const auto it = mStateConverters.find(pResource);
    if (it == mStateConverters.end()) return nullptr;
    return &it->second;
}

std::unordered_map<D3D12Resource*, std::vector<ResourceState>, HashPtrAsTyped<D3D12Resource*>> sGlobalResourceStates = {};