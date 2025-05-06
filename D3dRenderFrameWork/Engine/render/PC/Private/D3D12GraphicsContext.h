#pragma once
#ifdef WIN32
#include "Engine/render/RHIDefination.h"
#include "ResourceStateTracker.h"
#include "D3D12CommandContext.h"

class D3D12DepthStencil;
class D3D12RenderTarget;
class D3D12RingBufferAllocator;
class D3D12Device;
class D3D12Fence;
class D3D12CommandContext;

class D3D12GraphicsContext : public RHIGraphicsContext
{
    friend class D3D12RHI;
public:
    void Initialize(D3D12CommandContext* pGraphicCommandContext, ID3D12CommandAllocator* pCommandAllocator, const D3D12Resource* pStagingBufferPool);
    void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) override;
    void UpdateTexture(RHITextureWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) override;
    void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) override;
    void ClearRenderTarget(const RHIRenderTarget* pRenderTarget, const Float4& clearColor, const Rect* clearRects, uint32_t numRects) override;
    void ClearDepthStencil(const RHIDepthStencil* pDepthStencil, bool clearDepth, bool clearStencil, float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects) override;
    void SetPipelineState(const PipelineInitializer& initializer) override;
    void SetConstantBuffers(uint8_t baseSlot, uint8_t numSlots, RHIConstantBuffer* pConstants[]) override;
    void SetConstantBuffer(uint8_t slot, RHIConstantBuffer* pConstants) override;
    void SetTextures(uint8_t baseSlot, uint8_t numSlots, RHINativeTexture* textures[]) override;
    void SetTexture(uint8_t slot, RHINativeTexture* textures) override;
    void SetViewPorts(Viewport* viewports, uint32_t numViewports) override;
    void SetScissorRect(Rect* scissorRects, uint32_t numScissorRects) override;
    void SetRenderTargetsAndDepthStencil(RHIRenderTarget** renderTargets, uint32_t numRenderTargets, RHIDepthStencil* depthStencilTarget) override;
    void SetVertexBuffers(RHIVertexBuffer** vertexBuffers, uint8_t numVertexBuffers) override;
    void SetIndexBuffer(RHIIndexBuffer* pIndexBuffer) override;
    void DrawIndexed(uint32_t indexPerInstance, uint32_t baseIndex) override;
    void DrawIndexedInstanced(uint32_t indexPerInstance, uint32_t baseIndex, uint32_t instanceCount) override;
    void InsertFence(RHIFence* pFence, uint64_t semaphore) override;

    void BeginDrawCall() override;
	// push the dcs submitted since last call to EndDrawCalls to command list,
    // call this function before changing any states of graphic pipeline.
    void EndDrawCalls() override;
    //void Execute() override;
    //void ExecuteWithSync(RHIFence* pFence, uint64_t semaphore) override;

    void Reset(ID3D12GraphicsCommandList* pCommandList);
    void TransitionResource(const D3D12Resource* pResource, ResourceState dstState) const;

private:
    struct DrawCallParam
    {
	    DrawCallParam() = default;

	    DrawCallParam(uint32_t indexPerInstance, uint32_t baseIndex, uint32_t numInstances)
		    : mIndexPerInstance(indexPerInstance),
		      mBaseIndex(baseIndex),
		      mNumInstances(numInstances)
	    {
	    }

	    uint32_t mIndexPerInstance;
        uint32_t mBaseIndex;
        uint32_t mNumInstances;
    };
    const D3D12Resource* mStagingBufferPool;
	D3D12CommandContext* mCommandContext;
    const D3D12RootSignature* mRootSignature;

    ID3D12GraphicsCommandList* mCommandList;
    ID3D12CommandAllocator* mCommandAllocator;
    //std::unique_ptr<D3D12Fence> mFence;
    //uint64_t mSemaphore;

    std::unique_ptr<ResourceStateTracker> mResourceStateTracker;    //TODO: Pooling
    std::unique_ptr<D3D12DescriptorHandle[]> mDescriptorHandles;
    D3D12DescriptorHandle* mTextureHandles;
    std::vector<std::pair<D3D12Fence*, uint64_t>> mSynchronizes;    // TODO:
    std::vector<DrawCallParam> mDrawCalls;
};

class D3D12CopyContext : public RHICopyContext
{
    friend class D3D12RHI;
public:
    void Initialize(const D3D12Resource* pStagingBufferPool, ID3D12CommandAllocator* pDedicatedAllocator);
    void Reset(ID3D12GraphicsCommandList* pCommandList);
    //void SetCommandContext(D3D12CopyCommandContext* pCommandContext);
    void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) override;
    void UpdateTexture(RHINativeTexture* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) override;
    void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation,
                     const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) override;
    void InsertFence(RHIFence* pFence, uint64_t semaphore) override;

private:
    const D3D12Resource* mStagingBufferPool;
    //D3D12CopyCommandContext* mCopyCommandContext;
    ID3D12CommandAllocator* mCommandAllocator;
    ID3D12GraphicsCommandList* mCommandList;
    std::unique_ptr<ResourceStateTracker> mResourceStateTracker;
    std::vector<std::pair<D3D12Fence*, uint64_t>> mSynchronizes;    // TODO:
    //std::unique_ptr<D3D12Fence> mDedicatedFence;
    //uint64_t mSemaphore;
};

class D3D12ComputeContext : public RHIComputeContext
{
    friend class D3D12RHI;
};
#endif