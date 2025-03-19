#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/RenderResource/RenderTexture.h>
#include <Engine/render/PC/RenderResource/StaticBuffer.h>
#include <Engine/render/PC/RenderResource/DynamicBuffer.h>

class FrameResourceAllocator;
struct Fence;
class DynamicBuffer;
struct FrameResource;

enum class AllocationType : uint8_t
{
    PERSISTENT,
    PER_FRAME,
    TEMP,
};

class FrameResourceAllocator
{
    struct BufferDesc
    {
        uint64_t mSize;
    };

    struct RenderTargetDesc
    {
        TextureType mType;
        uint64_t mWidth;
        uint64_t mHeight;
        uint32_t mDepth;
        TextureFormat mFormat;
        uint8_t mNumMips;
        uint8_t mSampleCount;
        uint8_t mSampleQuality;
        bool mIsDynamic;
        bool mAllowSimultaneous;
    };

public:
    void ExecutePendingAllocationsForFrame(FrameResource& frameResource) const;
    
    uint64_t allocDynamicBuffer(AllocationType allocationType, uint64_t size);
    uint64_t allocStaticBuffer(AllocationType allocationType, uint64_t size);
    uint64_t allocRenderTexture(AllocationType allocationType,
                                      TextureType type, uint64_t width, uint64_t height,
                                      uint32_t depth,
                                      TextureFormat format,
                                      uint8_t numMips = 1, uint8_t sampleCount = 1,
                                      uint8_t sampleQuality = 0,
                                      bool isDynamic = false, bool allowSimultaneous = false);
    FrameResourceAllocator();
    ~FrameResourceAllocator();

    DELETE_COPY_OPERATOR(FrameResourceAllocator)
    DELETE_MOVE_OPERATOR(FrameResourceAllocator)
    DELETE_MOVE_CONSTRUCTOR(FrameResourceAllocator)
    DELETE_COPY_CONSTRUCTOR(FrameResourceAllocator)
    
private:
    std::vector<BufferDesc> mDynamicBufferDescriptors;
    std::vector<BufferDesc> mStaticBufferDescriptors;
    std::vector<RenderTargetDesc> mRenderTargetDescriptors;
};

FrameResource gCreateFrameResource(RenderTexture&& rt);

struct FrameResource
{
    FrameResource(RenderTexture&& rt, ID3D12CommandAllocator* pCommandAllocator);
    
    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    RenderTexture mRenderTexture;
    uint64_t mFenceValue = 0;
    
    std::vector<DynamicBuffer> mDynamicBuffers;
    std::vector<StaticBuffer> mStaticBuffers;
    std::vector<RenderTexture> mRenderTextures;
};
#endif