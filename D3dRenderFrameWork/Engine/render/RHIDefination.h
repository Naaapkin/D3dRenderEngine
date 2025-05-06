#pragma once
#include "RHIDescriptors.h"
#include "Engine/pch.h"

struct PipelineInitializer;

class RHIObject
{
public:
    RHIObject() = default;
    virtual ~RHIObject() = default;
    
    NON_COPYABLE(RHIObject)
    DEFAULT_MOVE_CONSTRUCTOR(RHIObject)
    DEFAULT_MOVE_OPERATOR(RHIObject)
};

class RHINativeResource: public RHIObject
{
public:
    virtual void Release() = 0;
    RHINativeResource() = default;
};

class RHINativeBuffer : public RHINativeResource
{
public:
    RHIBufferDesc GetDesc() const { return mDesc; };
    virtual uint32_t BufferSize() const = 0;
    RHINativeBuffer() = default;
    RHINativeBuffer(const RHIBufferDesc& desc) : RHINativeResource(), mDesc(desc) { }

protected:
    RHIBufferDesc mDesc;
};

class RHINativeTexture : public RHINativeResource
{
public:
    const RHITextureDesc& GetDesc() const { return mDesc; }
    RHINativeTexture() = default;
    RHINativeTexture(const RHITextureDesc& desc) : RHINativeResource(), mDesc(desc) { }

protected:
    RHITextureDesc mDesc;
};

class RHIBufferWrapper
{
public:
    RHINativeBuffer* GetBuffer() const { return mBuffer.get(); }
    RHIBufferWrapper() = default;
    RHIBufferWrapper(std::unique_ptr<RHINativeBuffer>&& pBuffer) : mBuffer(std::move(pBuffer)) { }

protected:
    std::unique_ptr<RHINativeBuffer> mBuffer;
};

class RHITextureWrapper
{
public:
    RHINativeTexture* GetTexture() const { return mTexture.get(); }
    RHITextureWrapper() = default;
    RHITextureWrapper(std::unique_ptr<RHINativeTexture>&& pTexture) : mTexture(std::move(pTexture)) { }

protected:
    std::unique_ptr<RHINativeTexture> mTexture;
};

#define DERIVE_BUFFER_WRAPPER(className) \
class className : public RHIObject, public RHIBufferWrapper \
{ \
public: \
	className() = default; \
	className(std::unique_ptr<RHINativeBuffer> pBuffer) : RHIObject(), RHIBufferWrapper(std::move(pBuffer)) { } \
};

#define DERIVE_TEXTURE_WRAPPER_CONSTRUCTOR(className) \
class className : public RHIObject, public RHITextureWrapper \
{ \
public: \
	className() = default; \
	className(std::unique_ptr<RHINativeTexture> pTexture) : RHIObject(), RHITextureWrapper(std::move(pTexture)) { } \
};

DERIVE_BUFFER_WRAPPER(RHIConstantBuffer);
DERIVE_BUFFER_WRAPPER(RHIStagingBuffer);
DERIVE_BUFFER_WRAPPER(RHIVertexBuffer);
DERIVE_BUFFER_WRAPPER(RHIIndexBuffer);

class RHIRenderTarget : public RHINativeResource
{
public:
    RHITextureDesc GetTextureDesc() const { return mDesc; }
    uint32_t GetWidth() const { return mDesc.mWidth; }
    uint32_t GetHeight() const { return mDesc.mHeight; }
    Format GetFormat() const { return mDesc.mFormat; }
    bool MSAAEnabled() const { return mDesc.mSampleCount > 1; }
    RHIRenderTarget() = default;
    RHIRenderTarget(const RHITextureDesc& desc) : RHINativeResource(), mDesc(desc) { }
    
private:
    RHITextureDesc mDesc;
};

class RHIDepthStencil : public RHINativeResource
{
public:
    RHITextureDesc GetTextureDesc() const { return mDesc; }
    uint32_t GetWidth() const { return mDesc.mWidth; }
    uint32_t GetHeight() const { return mDesc.mHeight; }
    Format GetFormat() const { return mDesc.mFormat; }
    bool MSAAEnabled() const { return mDesc.mSampleCount > 1; }
    bool MipmapEnabled() const { return mDesc.mMipLevels > 1; }
    RHIDepthStencil() = default;
    RHIDepthStencil(const RHITextureDesc& desc) : RHINativeResource(), mDesc(desc) { }
    
private:
    RHITextureDesc mDesc;
};

class RHIFence : public RHIObject
{
public:
    virtual uint64_t GetValue() const = 0;
    virtual void Wait(uint64_t value) const = 0;
    RHIFence() = default;
};

// class RHICommandQueue : public RHIObject
// {
// public:
//     virtual void SetFence(RHIFence* pFence) = 0;
//     virtual void Signal(uint64_t semaphore) const = 0;
//     virtual void InsertFence(uint64_t semaphore) const = 0;
//     virtual const RHIFence* GetFence() const = 0;
//     virtual uint64_t GetGPUSemaphore() const = 0;
//     virtual void ExecuteCommands(const std::vector<DynamicLinearAllocator::Handle<RHICommand>>& commands) = 0;
//     RHICommandQueue() = default;
//     RHICommandQueue(const std::string& name) : RHIObject(name) { }
// };

class RHICopyContext : public RHIObject
{
public:
    virtual void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) = 0;
    virtual void UpdateTexture(RHINativeTexture* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) = 0;
    virtual void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) = 0;
    virtual void InsertFence(RHIFence* pFence, uint64_t semaphore) = 0;
};

class RHIGraphicsContext : public RHIObject
{
public:
    virtual void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) = 0;
    virtual void UpdateTexture(RHITextureWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) = 0;
    virtual void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) = 0;
    virtual void ClearRenderTarget(const RHIRenderTarget* pRenderTarget, const Float4& clearColor, const Rect* clearRects, uint32_t numRects) = 0;
    virtual void ClearDepthStencil(const RHIDepthStencil* pDepthStencil, bool clearDepth, bool clearStencil, float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects) = 0;
    virtual void SetPipelineState(const PipelineInitializer& initializer) = 0;
    virtual void SetConstantBuffers(uint8_t baseSlot, uint8_t numSots, RHIConstantBuffer* pConstants[]) = 0;
    virtual void SetConstantBuffer(uint8_t slot, RHIConstantBuffer* pConstants) = 0;
    virtual void SetTextures(uint8_t baseSlot, uint8_t numSots, RHINativeTexture* textures[]) = 0;
    virtual void SetTexture(uint8_t slot, RHINativeTexture* textures) = 0;
    virtual void SetViewPorts(Viewport* viewports, uint32_t numViewports) = 0;
    virtual void SetScissorRect(Rect* scissorRects, uint32_t numScissorRects) = 0;
    virtual void SetRenderTargetsAndDepthStencil(RHIRenderTarget** renderTargets, uint32_t numRenderTargets, RHIDepthStencil* depthStencilTarget) = 0;
    virtual void SetVertexBuffers(RHIVertexBuffer** vertexBuffers, uint8_t numVertexBuffers) = 0;
    virtual void SetIndexBuffer(RHIIndexBuffer* pIndexBuffer) = 0;
    virtual void DrawIndexed(uint32_t indexPerInstance, uint32_t baseIndex) = 0;
    virtual void DrawIndexedInstanced(uint32_t indexPerInstance, uint32_t baseIndex, uint32_t instanceCount) = 0;
    virtual void EndDrawCalls() = 0;
    virtual void InsertFence(RHIFence* pFence, uint64_t semaphore) = 0;
    virtual void BeginDrawCall() = 0;

    RHIGraphicsContext() = default;
};

class RHIComputeContext : public RHIObject
{
};


class RHISwapChain : public RHIObject
{
public:
    virtual RHIRenderTarget* GetCurrentColorTexture() = 0;
    virtual RHIRenderTarget* GetColorTexture(uint8_t backBufferIndex) = 0;
    virtual RHITextureDesc GetBackBufferDesc() const = 0;
    virtual void BeginFrame(RHIGraphicsContext* pContext) = 0;
    virtual void EndFrame(RHIGraphicsContext* pContext) = 0;
    virtual void Present()
    {
        mBackBufferIndex = (mBackBufferIndex + 1) % mNumBackBuffers;
    }
    uint8_t GetBackBufferIndex() const
    {
        return mBackBufferIndex;
    }
    uint8_t GetNumBackBuffers() const
    {
        return mNumBackBuffers;
    }
    RHISwapChain() = default;
    RHISwapChain(uint8_t numBackBuffers) : RHIObject(), mNumBackBuffers(numBackBuffers), mBackBufferIndex(0)
    {
    }

protected:
    uint8_t mNumBackBuffers;
    uint8_t mBackBufferIndex;
};