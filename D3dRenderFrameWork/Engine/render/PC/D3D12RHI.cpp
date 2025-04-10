#include "D3D12RHI.h"

void D3D12RHI::Initialize()
{
    
    RHI::Initialize();
}

RHIBuffer* D3D12RHI::RHIAllocStagingBuffer(const RHIBufferDesc& desc)
{
    
    mDevice->CreateCommitedResource()
}

RHITexture* D3D12RHI::RHIAllocStagingTexture(const RHITextureDesc& desc)
{
}

RHIBuffer* D3D12RHI::RHIAllocVertexBuffer(uint64_t vertexSize, uint64_t numVertices)
{
}

RHIBuffer* D3D12RHI::RHIAllocIndexBuffer(uint64_t numIndices)
{
}

RHIBuffer* D3D12RHI::RHIAllocConstantBuffer(uint64_t size)
{
}

RHITexture* D3D12RHI::RHIAllocTexture(const RHITextureDesc& desc)
{
}

RHIFrameBuffer* D3D12RHI::RHIAllocFrameBuffer(const RHITexture2D* rt, FrameBufferType type)
{
}

void D3D12RHI::RHIUpdateVertexBuffer(RHIBuffer* pVertexBuffer, const void* pData, uint64_t numVertices)
{
}

void D3D12RHI::RHIUpdateIndexBuffer(RHIBuffer* pIndexBuffer, const uint32_t* pData, uint64_t numIndices)
{
}

void D3D12RHI::RHIUpdateStagingBuffer(RHIBuffer* pBuffer, const void* pData, uint64_t pos, uint64_t size)
{
}

void D3D12RHI::RHICopyFromStagingBuffer(RHIBuffer* pBuffer, RHIBuffer* pStagingBuffer)
{
}

RHICommandList* D3D12RHI::RHIGetCommandList(CommandQueueType type)
{
}

RHICommandQueue* D3D12RHI::RHIGetCommandQueue(CommandQueueType type)
{
}

void D3D12RHI::RHISetViewport(RHIGraphicCommandList* pCommandList, const Viewport* viewports, uint8_t numViewports)
{
}

void D3D12RHI::RHISetScissorRect(RHIGraphicCommandList* pCommandList, const Rect* scissorRect, uint8_t numScissorRects)
{
}

void D3D12RHI::RHISetVertexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pVertexBuffer)
{
}

void D3D12RHI::RHISetIndexBuffer(RHIGraphicCommandList* pCommandList, RHIBuffer* pIndexBuffer)
{
}

void D3D12RHI::RHIDrawIndexed(RHIGraphicCommandList* pCommandList, uint32_t numIndices, uint32_t startIndex)
{
}

void D3D12RHI::RHIExecuteCommandList(RHIGraphicCommandList* pCommandList)
{
}

D3D12RHI::D3D12RHI()
{
}

D3D12RHI::~D3D12RHI()
{
}
