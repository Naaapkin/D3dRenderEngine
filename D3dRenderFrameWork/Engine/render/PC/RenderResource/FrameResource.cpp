#ifdef WIN32
#include <Engine/common/Exception.h>
#include <Engine/render/PC/Core/D3dGraphicContext.h>
#include <Engine/render/PC/RenderResource/FrameResource.h>

void FrameResourceAllocator::ExecutePendingAllocationsForFrame(FrameResource& frameResource) const
{
    uint64_t num = mDynamicBufferDescriptors.size();
    auto& dynamicBuffers = frameResource.mDynamicBuffers;
    dynamicBuffers.reserve(num);
    for (uint64_t i = 0; i < num; ++i)
    {
        dynamicBuffers.emplace_back(::gCreateDynamicBuffer(mDynamicBufferDescriptors[i].mSize));
    }
    num = mStaticBufferDescriptors.size();
    auto& staticBuffers = frameResource.mStaticBuffers;
    staticBuffers.reserve(num);
    for (uint64_t i = 0; i < num; ++i)
    {
        staticBuffers.emplace_back(::gCreateStaticBuffer(mStaticBufferDescriptors[i].mSize));
    }
    num = mRenderTargetDescriptors.size();
    auto& renderTextures = frameResource.mRenderTextures;
    renderTextures.reserve(num);
    for (uint64_t i = 0; i < num; ++i)
    {
        auto& desc = mRenderTargetDescriptors[i];
        renderTextures.emplace_back(::gCreateRenderTexture(desc.mType, desc.mWidth, desc.mHeight, desc.mDepth,
                                                           desc.mFormat, desc.mNumMips, desc.mSampleCount,
                                                           desc.mSampleQuality, desc.mIsDynamic,
                                                           desc.mAllowSimultaneous));
    }
}

// TODO: implement "TEMP" case
// when allocationType is PERSISTENT, the life time of resource should be managed by caller, function will return a pointer to the resource instantly.
// when allocationType is PER_FRAME, resource will be alive through the entire program, every backresource will hold a independent resource, function will return a resource handle which is invalid before the initialization of engine finished.
// when allocationType is TEMP, function behaves like the PERSISTENT case, but the life time will be managed by engine, which means resource only lives one frame.
uint64_t FrameResourceAllocator::allocDynamicBuffer(AllocationType allocationType, uint64_t size)
{
    switch (allocationType)
    {
    case AllocationType::PER_FRAME:
        mDynamicBufferDescriptors.emplace_back(BufferDesc{ size });
        return mDynamicBufferDescriptors.size();
    case AllocationType::PERSISTENT:
        return reinterpret_cast<uint64_t>(new DynamicBuffer(::gCreateDynamicBuffer(size)));
    case AllocationType::TEMP:
    default:
        return NULL;
    }
}

// TODO: implement "TEMP" case
// when allocationType is PERSISTENT, the life time of resource should be managed by caller, function will return a pointer to the resource instantly.
// when allocationType is PER_FRAME, resource will be alive through the entire program, every backresource will hold a independent resource, function will return a resource handle which is invalid before the initialization of engine finished.
// when allocationType is TEMP, function behaves like the PERSISTENT case, but the life time will be managed by engine, which means resource only lives one frame.
uint64_t FrameResourceAllocator::allocStaticBuffer(AllocationType allocationType, uint64_t size)
{
    switch (allocationType)
    {
    case AllocationType::PER_FRAME:
        mStaticBufferDescriptors.emplace_back(BufferDesc{ size });
        return mStaticBufferDescriptors.size();
    case AllocationType::PERSISTENT:
        return reinterpret_cast<uint64_t>(new StaticBuffer(::gCreateStaticBuffer(size)));
    case AllocationType::TEMP:
    default:
        return NULL;
    }
}

// TODO: implement "TEMP" case
// when allocationType is PERSISTENT, the life time of resource should be managed by caller, function will return a pointer to the resource instantly.
// when allocationType is PER_FRAME, resource will be alive through the entire program, every backresource will hold a independent resource, function will return a resource handle which is invalid before the initialization of engine finished.
// when allocationType is TEMP, function behaves like the PERSISTENT case, but the life time will be managed by engine, which means resource only lives one frame.
uint64_t FrameResourceAllocator::allocRenderTexture(AllocationType allocationType, TextureType type, uint64_t width,
    uint64_t height, uint32_t depth, TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality,
    bool isDynamic, bool allowSimultaneous)
{
    switch (allocationType)
    {
    case AllocationType::PER_FRAME:
        mRenderTargetDescriptors.emplace_back(RenderTargetDesc{
            type, width, height, depth, format, numMips, sampleCount, sampleQuality, isDynamic, allowSimultaneous
        });
        return mRenderTargetDescriptors.size();
    case AllocationType::PERSISTENT:
        return reinterpret_cast<uint64_t>(new RenderTexture(::gCreateRenderTexture(
            type, width, height, depth, format, numMips,
            sampleCount,
            sampleQuality, isDynamic, allowSimultaneous)));
    case AllocationType::TEMP:
    default:
        return NULL;
    }
}

FrameResourceAllocator::FrameResourceAllocator() = default;

FrameResourceAllocator::~FrameResourceAllocator() = default;

FrameResource gCreateFrameResource(RenderTexture&& rt)
{
    ID3D12Device* pDevice = gGraphicContext()->DeviceHandle();
    ID3D12CommandAllocator* pCommandAllocator;
    ThrowIfFailed(
        pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&pCommandAllocator))
    );
    return { std::move(rt), pCommandAllocator };
}

FrameResource::FrameResource(RenderTexture&& rt, ID3D12CommandAllocator* pCommandAllocator) :
    mCommandAllocator(pCommandAllocator),
    mRenderTexture(std::move(rt)) { }
#endif
