#pragma once
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/Render/RHIDefination.h"

class Blob;
class RHIShader;
class AHCommandList;    // TODO:

class RHI : NonCopyable
{
    friend RHI* GetRHI();
    
public:
    using RenderPass = std::function<void(AHCommandList&)>;
    virtual void Initialize();
    virtual std::unique_ptr<RHIShader>          RHICompileShader(const Blob& binary, ShaderType activeShaders, const std::string* path = nullptr) = 0;
    virtual std::unique_ptr<RHISwapChain>       RHICreateSwapChain(const RHISwapChainDesc& desc) = 0;
    //virtual std::unique_ptr<RHINativeBuffer>      RHIAllocConstantBuffer(uint64_t size) = 0;
    virtual std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingBuffer(uint64_t size) = 0;
    virtual std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingBuffer(const RHITextureDesc& desc, uint8_t mipmap) = 0;
    virtual std::unique_ptr<RHIConstantBuffer>  RHIAllocConstantBuffer(uint64_t size) = 0;
    virtual std::unique_ptr<RHIInstanceBuffer>  RHIAllocInstanceBuffer(uint64_t size) = 0;
    virtual std::unique_ptr<RHINativeTexture>   RHIAllocTexture(RHITextureDesc desc) = 0;
    virtual std::unique_ptr<RHIDepthStencil>    RHIAllocDepthStencil(RHITextureDesc desc) = 0;
    virtual std::unique_ptr<RHIRenderTarget>    RHIAllocRenderTarget(RHITextureDesc desc) = 0;
    virtual std::unique_ptr<RHIVertexBuffer>    RHIAllocVertexBuffer(uint32_t vertexSize, uint32_t numVertices) = 0;
    virtual std::unique_ptr<RHIIndexBuffer>     RHIAllocIndexBuffer(uint64_t numIndices, Format indexFormat) = 0;
    virtual std::unique_ptr<RHIFence>           RHICreateFence() = 0;

    virtual void RHIUpdateStagingBuffer(RHIStagingBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) = 0;
    virtual void RHIUpdateStagingBuffer(RHIStagingBuffer* pStagingBuffer, const RHITextureDesc& desc, const void* pData, uint8_t mipmap) = 0;
    virtual void RHIUpdateConstantBuffer(RHIConstantBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) = 0;
    virtual void RHIUpdateInstanceBuffer(RHIInstanceBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) = 0;
    virtual void RHIReleaseConstantBuffer(RHIConstantBuffer* pBuffer) = 0;
    virtual void RHIReleaseInstanceBuffer(RHIInstanceBuffer* pBuffer) = 0;

    virtual void RHICreateGraphicsContext(RHIGraphicsContext** ppContext) = 0;
    virtual void RHICreateCopyContext(RHICopyContext** ppContext) = 0;
    virtual void RHICreateComputeContext(RHIComputeContext** ppContext) = 0;
    virtual void RHIResetGraphicsContext(RHIGraphicsContext* pContext) = 0;
    virtual void RHIResetCopyContext(RHICopyContext* pContext) const = 0;
    virtual void RHISubmitRenderCommands(RHIGraphicsContext* pContext) = 0;
    virtual void RHISyncGraphicContext(RHIFence* pFence, uint64_t semaphore) = 0;
    virtual void RHISubmitCopyCommands(RHICopyContext* pContext) = 0;
    virtual void RHISyncCopyContext(RHIFence* pFence, uint64_t semaphore) const = 0;
    virtual void RHIBatchCopyCommands(RHICopyContext** pContexts, uint32_t numContexts) = 0;
    virtual void RHIReleaseGraphicsContext(RHIGraphicsContext* pContext) = 0;
    virtual void RHIReleaseCopyContext(RHICopyContext* pContext) = 0;
    
    // virtual void BeginFrame(RHIFrameResource* pFrameResource) = 0;
    // virtual void EndFrame() = 0;
    // virtual void AddRenderPass(const RenderPass& renderPass) = 0;
    
    virtual void Release() = 0;
    virtual ~RHI();

protected:
    bool mIsInitialized = false;
};

inline void RHI::Initialize()
{
    mIsInitialized = true;
}

inline RHI::~RHI() { }

RHI* GetRHI();
