#pragma once
#include <iosfwd>
#include <iosfwd>
#include <vector>
#include <vector>

#include "Blob.h"
#include "Engine/pch.h"
#include "Engine/common/Format.h"

class RHIObject
{
public:
    virtual void SetName(const std::string& name) = 0;
    virtual const std::string& GetName() const = 0;
    RHIObject() = default;
    virtual ~RHIObject() = default;

    DELETE_MOVE_OPERATOR(RHIObject);
    DELETE_MOVE_CONSTRUCTOR(RHIObject);
    DEFAULT_COPY_OPERATOR(RHIObject);
    DEFAULT_COPY_CONSTRUCTOR(RHIObject);
};

enum class ResourceType : uint8_t
{
    DYNAMIC,
    STATIC,
    SHADER_RESOURCE,
    UNORDERED_ACCESS,
    NONE,
};

enum class TextureDimension : uint8_t
{
    TEXTURE2D,
    TEXTURE_ARRAY,
    TEXTURE3D,
};

enum class FrameBufferType : uint8_t
{
    COLOR,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,
};

enum class CommandQueueType : uint8_t
{
    GRAPHIC,
    COPY,
    COMPUTE
};

struct RHIBufferDesc final
{
    RHIBufferDesc() : mSize(0), mStride(sizeof(float)), mResourceType(ResourceType::NONE) { }
    RHIBufferDesc(uint64_t size, uint64_t stride, ResourceType resourceType) : mSize(size), mStride(stride), mResourceType(resourceType) { }
    
    uint64_t mSize;
    uint64_t mStride;
    ResourceType mResourceType;
};

struct RHITextureDesc final
{
    RHITextureDesc() : mFormat(Format::UNKNOWN), mDimension(TextureDimension::TEXTURE2D), mWidth(0), mHeight(0), mDepth(0), mMipLevels(0),
                       mSampleCount(1), mSampleQuality(0)
    {
    }
    RHITextureDesc(Format format, TextureDimension dimension, uint32_t width, uint32_t height, uint32_t depth, uint8_t mipLevels,
                   uint8_t sampleCount, uint8_t sampleQuality) :
        mFormat(format), mDimension(dimension), mWidth(width), mHeight(height), mDepth(depth), mMipLevels(mipLevels),
        mSampleCount(sampleCount), mSampleQuality(sampleQuality)
    {
    };

    Format mFormat;
    TextureDimension mDimension;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;
    uint8_t mMipLevels;
    uint8_t mSampleCount;
    uint8_t mSampleQuality;
};

struct Viewport
{
    Viewport(float width, float height, float minDepth, float maxDepth)
        : mTop(0.0f), mLeft(0.0f), mWidth(width), mHeight(height),
          mMinDepth(minDepth), mMaxDepth(maxDepth) {}
    float mTop;
    float mLeft;
    float mWidth;
    float mHeight;
    float mMinDepth;
    float mMaxDepth;
};

struct Rect
{
    Rect(LONG left, LONG top, LONG right, LONG bottom)
        : mLeft(left), mTop(top), mRight(right), mBottom(bottom) {}
    LONG mLeft;
    LONG mTop;
    LONG mRight;
    LONG mBottom;
};

class RHIResource: public RHIObject
{
public:
    virtual void Release() = 0;
};

class RHIBuffer : public RHIResource
{
public:
    virtual RHIBufferDesc GetDesc() const = 0;
    virtual uint64_t GetSize() const = 0;
};

class RHITexture : public RHIResource
{
public:
    virtual const RHITextureDesc& GetDesc() const = 0;
};

class RHITexture2D : public RHITexture
{
public:
    virtual void GetSize(uint32_t& width, uint32_t& height) const = 0;
};

class RHITexture3D : public RHITexture
{
public:
    virtual void GetSize(uint32_t& width, uint32_t& height, uint32_t depth) const = 0;
};

class RHITextureArray : public RHITexture
{
public:
    virtual void GetSize(uint32_t& width, uint32_t& height, uint32_t arraySize) const = 0;
};

class RHIFrameBuffer : public RHIObject
{
public:
    FrameBufferType GetType() const { return mType; }
    void SetType(FrameBufferType type) { mType = type; }
    
    virtual RHITexture2D* GetAttachment() const = 0;
    RHIFrameBuffer() = default;
    RHIFrameBuffer(FrameBufferType type) : mType(type) { }
    
private:
    FrameBufferType mType;
};

class RHIFence : public RHIObject
{
public:
    virtual void Wait(uint64_t fencePoint) const = 0;
};

class RHICommandList : public RHIObject
{
public:
    virtual void Reset() = 0;
    virtual void Clear() = 0;
    virtual void Close() = 0;
};

class RHIGraphicCommandList : public RHICommandList
{
public:
    virtual void SetViewPort(const std::vector<__resharper_unknown_type>& viewports)= 0;
    virtual void SetScissorRect(const std::vector<Rect>& scissorRects)= 0;
    virtual void CopyResource(RHIResource* dstResource, RHIResource* srcResource) = 0;
private:
};

class RHICopyCommandList : public RHICommandList
{
public:
    virtual void CopyResource(RHIResource* dstResource, RHIResource* srcResource) = 0;
private:
};

class RHIComputeCommandList : public RHICommandList
{
public:
private:
};

class RHICommandQueue : public RHIObject
{
public:
    virtual void Signal(const RHIFence* pFence, uint64_t fenceValue) const = 0;
    virtual void Wait(const RHIFence* pFence, uint64_t fenceValue) const = 0;
    virtual void ExecuteCommandLists(RHIGraphicCommandList* commandLists, uint32_t numPendingLists) = 0;
};

class RHICommandContext : public RHIObject
{
public:
    // using RenderExeBuilder = std::function<void (*)(RHIGraphicCommandList*)>;
    // using CopyExeBuilder = std::function<void (*)(RHICopyCommandList*)>;
    // using ExecutionBuilder = std::function<void (*)(RHIGraphicCommandList*, RHICopyCommandList*)>;
    // virtual void AddAsyncTask(RenderExeBuilder renderExeBuilder) = 0;
    // virtual void AddAsyncTask(CopyExeBuilder renderExeBuilder) = 0;
    // virtual void AddAsyncTask(ExecutionBuilder renderExeBuilder) = 0;
    virtual RHIGraphicCommandList* GetGraphicCommandList() = 0;
    virtual RHICopyCommandList* GetCopyCommandList() = 0;
    virtual RHIComputeCommandList* GetComputeCommandList() = 0;
    virtual void Reset(RHICommandQueue* pCommandQueue) = 0;
    virtual void SetRenderTarget(RHITexture2D* renderTarget, uint8_t numRenderTargets) = 0;
    virtual void SetDepthStencil(RHITexture2D* depthStencil) = 0;
    virtual void ExecuteCommandList(RHIGraphicCommandList* pCommandList) = 0;
    virtual void ExecuteCommandList(RHICopyCommandList* pCommandList) = 0;
    virtual void ExecuteCommandList(RHIComputeCommandList* pCommandList) = 0;
};

class RHIFrameResource : public RHIObject
{
public:
    virtual void UpdatePassConstant(uint32_t slot, const Blob& data) = 0;
    virtual void UpdateMaterialConstant(uint32_t slot, const Blob& data) = 0;
    virtual void SetGlobalTextureResource(uint32_t slot, RHITexture* pTex) = 0;
};

template<typename PlatformRHI> 
static PlatformRHI& GetRHI()
{
    static PlatformRHI rhi{};
    if (!rhi.mIsInitialized) rhi.Initialize();
    return rhi;
}

class RHI : NonCopyable
{
    template<typename PlatformRHI>
    friend PlatformRHI& GetRHI();
    
public:
    virtual void Initialize() { mIsInitialized = true; }
    virtual RHIBuffer* RHIAllocStagingBuffer(const RHIBufferDesc& desc) = 0;
    virtual RHITexture* RHIAllocStagingTexture(const RHITextureDesc& desc) = 0;
    virtual RHIBuffer* RHIAllocVertexBuffer(uint64_t vertexSize, uint64_t numVertices) = 0;
    virtual RHIBuffer* RHIAllocIndexBuffer(uint64_t numIndices) = 0;
    virtual RHIBuffer* RHIAllocConstantBuffer(uint64_t size) = 0;
    virtual RHITexture* RHIAllocTexture(const RHITextureDesc& desc) = 0;
    virtual RHIFrameBuffer* RHIAllocFrameBuffer(const RHITexture2D* rt, FrameBufferType type) = 0;
    virtual void RHIUpdateVertexBuffer(RHIBuffer* pVertexBuffer, const void* pData, uint64_t numVertices) = 0;
    virtual void RHIUpdateIndexBuffer(RHIBuffer* pIndexBuffer, const uint32_t* pData, uint64_t numIndices) = 0;
    virtual void RHIUpdateStagingBuffer(RHIBuffer* pBuffer, const void* pData, uint64_t pos, uint64_t size) = 0;
    virtual void RHICopyFromStagingBuffer(RHIBuffer* pBuffer, RHIBuffer* pStagingBuffer) = 0;

    virtual RHICommandList* RHIGetCommandList(CommandQueueType type) = 0;
    virtual RHICommandQueue* RHIGetCommandQueue(CommandQueueType type) = 0;
    virtual void RHISetViewport(RHIGraphicCommandList* pCommandList, const Viewport* viewports, uint8_t numViewports) = 0;
    virtual void RHISetScissorRect(RHIGraphicCommandList* pCommandList, const Rect* scissorRect, uint8_t numScissorRects) = 0;
    virtual void RHISetVertexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pVertexBuffer) = 0;
    virtual void RHISetIndexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pIndexBuffer) = 0;
    virtual void RHIDrawIndexed(RHIGraphicCommandList* pCommandList, uint32_t numIndices, uint32_t startIndex) = 0;
    virtual void RHIExecuteCommandList(RHIGraphicCommandList* pCommandList) = 0;
    virtual ~RHI() = 0;

private:
    bool mIsInitialized = false;
};