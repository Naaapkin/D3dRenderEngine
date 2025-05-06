#pragma once
#include "Engine/memory/LinearAllocator.h"
#include "Engine/render/RHIDefination.h"
#include "Engine/render/PC/Native/D3D12Device.h"
#include "Engine/render/PC/Private/ResourceStateTracker.h"

#ifdef WIN32

class D3D12CommandNative;
class MaterialInstance;
class D3D12FrameResource;
class D3D12DescriptorHeap;
class RHICommand;
class D3D12CommandExecutor;
class D3D12FrameBuffer;
class RHIPixelShader;
class RHIGeometryShader;
class RHIDomainShader;
class RHIHullShader;
class RHIVertexShader;
class D3D12Command;

class D3D12CommandContext : D3D12DeviceChild, public RHICommandContext
{
public:
    void Reset() override;
    void CopyResource(RHIResource* pDst, RHIResource* pSrc) override;
    void CopyBuffer(RHIBuffer* pDst, RHIBuffer* pSrc, uint64_t size, uint64_t dstStart, uint64_t srcStart) override;
    void CopyTexture(RHITexture* pDst, RHIResource* pSrc, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& location) override;
    void SetViewPorts(Viewport* viewports, uint32_t numViewports) override;
    void SetScissorRect(Rect* scissorRects, uint32_t numScissorRects) override;
    void SetRenderTargetsAndDepthStencil(RHIFrameBuffer** renderTargets, uint32_t numRenderTargets, RHIFrameBuffer* depthStencilTarget) override;
    void ClearRenderTarget(RHIFrameBuffer* pRenderTarget, Float4 clearColor, const Rect* clearRects, uint32_t numRects) override;
    void ClearDepthStencil(RHIFrameBuffer* depthStencil, bool clearDepth, bool clearStencil, float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects) override;
    void BindVertexBuffers(RHIVertexBuffer* pVertexBuffer) override;
    void BindIndexBuffer(RHIIndexBuffer* pIndexBuffer) override;
    void SetPipelineState(const PipelineInitializer& pPipelineInitializer) override;
    void DrawIndexedInstanced(uint32_t indexPerInstance, uint32_t startIndexLocation, uint32_t instanceCount) override;
    std::vector<LinearAllocator::Handle<RHICommand>> Finalize(CommandListType type) override;
    
    void TransitionResource(RHIResource* pResource, uint32_t subResource, ResourceState state);
    void TransitionResource(RHIResource* pResource, ResourceState state);
    void SetRootSignatureParas(uint8_t materialConstantPara, uint8_t materialTexturePara);
    void InsertNativeCommand(const std::function<void(D3D12CommandNative* pCommand)>& injection);

    D3D12CommandContext();
    D3D12CommandContext(D3D12Device* parent);

    GUID Guid() const override = delete;

private:
    // root signature
    uint8_t mMaterialCBufferPara;
    uint8_t mMaterialTexturePara;
    //
    
    ResourceStateTracker mResourceStateTracker;
    LinearAllocator mAllocator;
    std::vector<LinearAllocator::Handle<RHICommand>> mCommandsBuffer;
};
#endif
