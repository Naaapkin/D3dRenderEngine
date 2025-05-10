#pragma once
#ifdef WIN32
#include "Engine/Render/RHIDefination.h"
#include "ResourceStateTracker.h"
#include "D3D12CommandContext.h"

class D3D12FrameBuffer;
class D3D12RingBufferAllocator;
class D3D12Device;
class D3D12Fence;
class D3D12CommandContext;

class D3D12GraphicsContext : public RHIGraphicsContext
{
    friend class D3D12RHI;
public:
    void Initialize(D3D12CommandContext* pGraphicCommandContext, ID3D12CommandAllocator* pCommandAllocator, const D3D12Resource*
                    pStagingBufferPool);
    std::unique_ptr<RHIConstantBuffer> AllocTempConstantBuffer(uint16_t size) override;
    std::unique_ptr<RHIInstanceBuffer> AllocTempInstanceBuffer(uint32_t size, uint32_t stride) override;
    void UpdateConstantBuffer(RHIConstantBuffer* pBuffer, void* pData, uint64_t offset, uint64_t size) override;
    void UpdateInstanceBuffer(RHIInstanceBuffer* pBuffer, void* pData, uint64_t offset, uint64_t size) override;
    void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) override;
    void UpdateTexture(RHITextureWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) override;
    void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) override;
    void ClearRenderTarget(const RHIRenderTarget* pRenderTarget, const Float4& clearColor, const Rect* clearRects, uint32_t numRects) override;
    void ClearDepthStencil(const RHIDepthStencil* pDepthStencil, bool clearDepth, bool clearStencil, float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects) override;
    void SetPipelineState(const PipelineInitializer& initializer) override;
    void SetConstantBuffers(uint8_t baseSlot, uint8_t numSlots, RHIConstantBuffer* pConstants[]) override;
    void SetConstantBuffer(uint8_t slot, RHIConstantBuffer* pConstants) override;
    void SetInstanceBuffer(uint8_t slot, RHIInstanceBuffer* pInstanceBuffer) override;
    void SetTextures(uint8_t baseSlot, uint8_t numSlots, RHINativeTexture* textures[]) override;
    void SetTexture(uint8_t slot, RHINativeTexture* textures) override;
    void SetViewPorts(Viewport* viewports, uint32_t numViewports) override;
    void SetScissorRect(Rect* scissorRects, uint32_t numScissorRects) override;
    void SetRenderTargetsAndDepthStencil(RHIRenderTarget* const* renderTargets, uint32_t numRenderTargets, RHIDepthStencil const* depthStencilTarget) override;
    void SetVertexBuffers(RHIVertexBuffer** vertexBuffers, uint8_t numVertexBuffers) override;
    void SetIndexBuffer(const RHIIndexBuffer* pIndexBuffer) override;
    void DrawInstanced(uint32_t verticesPerInstance, uint32_t baseVertex, uint32_t instanceCount, uint32_t baseInstance) override;
    void DrawIndexedInstanced(uint32_t indicesPerInstance, uint32_t baseIndex, uint32_t baseVertex, uint32_t instanceCount, uint32_t baseInstance) override;
    void InsertFence(RHIFence* pFence, uint64_t semaphore) override;

    void BeginBinding() override;
	// push the dcs submitted since last call to EndBinding to command list,
    // call this function before changing any states of graphic pipeline.
    void EndBinding() override;
    //void Execute() override;
    //void ExecuteWithSync(RHIFence* pFence, uint64_t semaphore) override;

    void Reset(ID3D12GraphicsCommandList* pCommandList);
    void TransitionResource(const D3D12Resource* pResource, ResourceState dstState) const;

private:
    struct DrawCallParam
    {
	    DrawCallParam() = default;

	    DrawCallParam(uint32_t numVerticesPerInstance, uint32_t baseIndex, uint32_t baseVertex, uint32_t numInstances,
		    uint32_t baseInstance, bool indexed)
		    : mNumVerticesPerInstance(numVerticesPerInstance),
		      mBaseIndex(baseIndex),
		      mBaseVertex(baseVertex),
		      mNumInstances(numInstances),
		      mBaseInstance(baseInstance),
    	      mIndexed(indexed)
	    {
	    }

	    uint32_t mNumVerticesPerInstance;
        uint32_t mBaseIndex;
        uint32_t mBaseVertex;
        uint32_t mNumInstances;
        uint32_t mBaseInstance;
        bool mIndexed;
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
    D3D12DescriptorHandle* mResourceHandles;
    std::vector<std::pair<D3D12Fence*, uint64_t>> mSynchronizes;    // TODO:
    //std::vector<DrawCallParam> mDrawCalls;
};

class D3D12CopyContext : public RHICopyContext
{
    friend class D3D12RHI;
public:
    void Initialize(const D3D12Resource* pStagingBufferPool, ID3D12CommandAllocator* pDedicatedAllocator);
    void Reset(ID3D12GraphicsCommandList* pCommandList);
    //void SetCommandContext(D3D12CopyCommandContext* pCommandContext);
    void UpdateBuffer(RHIConstantBuffer* pCBuffer, void* pData, uint64_t offset, uint64_t size) override;
    void UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart, uint64_t srcStart) override;
    void UpdateTexture(RHINativeTexture* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap) override;
    void CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation,
                     const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth) override;
    void InsertFence(RHIFence* pFence, uint64_t semaphore) override;

private:
    //D3D12CopyCommandContext* mCopyCommandContext;
    ID3D12CommandAllocator* mCommandAllocator;
    ID3D12GraphicsCommandList* mCommandList;
    const D3D12Resource* mStagingBufferPool;
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