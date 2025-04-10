#pragma once
#include "Resource/D3D12Resources.h"

class D3D12RHI;

D3D12RHI* gD3D12RHI = nullptr;

class D3D12RHI : public RHI
{
public:
    void Initialize() override;
    RHIBuffer* RHIAllocStagingBuffer(const RHIBufferDesc& desc) override;
    RHITexture* RHIAllocStagingTexture(const RHITextureDesc& desc) override;
    RHIBuffer* RHIAllocVertexBuffer(uint64_t vertexSize, uint64_t numVertices) override;
    RHIBuffer* RHIAllocIndexBuffer(uint64_t numIndices) override;
    RHIBuffer* RHIAllocConstantBuffer(uint64_t size) override;
    RHITexture* RHIAllocTexture(const RHITextureDesc& desc) override;
    RHIFrameBuffer* RHIAllocFrameBuffer(const RHITexture2D* rt, FrameBufferType type) override;
    void RHIUpdateVertexBuffer(RHIBuffer* pVertexBuffer, const void* pData, uint64_t numVertices) override;
    void RHIUpdateIndexBuffer(RHIBuffer* pIndexBuffer, const uint32_t* pData, uint64_t numIndices) override;
    void RHIUpdateStagingBuffer(RHIBuffer* pBuffer, const void* pData, uint64_t pos, uint64_t size) override;
    void RHICopyFromStagingBuffer(RHIBuffer* pBuffer, RHIBuffer* pStagingBuffer) override;
    RHICommandList* RHIGetCommandList(CommandQueueType type) override;
    RHICommandQueue* RHIGetCommandQueue(CommandQueueType type) override;
    void RHISetViewport(RHIGraphicCommandList* pCommandList, const Viewport* viewports, uint8_t numViewports) override;
    void RHISetScissorRect(RHIGraphicCommandList* pCommandList, const Rect* scissorRect,
        uint8_t numScissorRects) override;
    void RHISetVertexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pVertexBuffer) override;
    void RHISetIndexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pIndexBuffer) override;
    void RHIDrawIndexed(RHIGraphicCommandList* pCommandList, uint32_t numIndices, uint32_t startIndex) override;
    void RHIExecuteCommandList(RHIGraphicCommandList* pCommandList) override;;
    D3D12RHI();
    ~D3D12RHI() override;

private:
    D3D12Device* mDevice;
};

template <>
inline D3D12RHI& GetRHI<D3D12RHI>()
{
    if (!gD3D12RHI) {
        gD3D12RHI = new D3D12RHI();
        gD3D12RHI->Initialize();
    }
    return *gD3D12RHI;
}