#include "Engine/math/math.h"
#ifdef WIN32
#include "D3D12GraphicsContext.h"

#include "Engine/Render/PC/Native/D3D12RootSignatureManager.h"
#include "Engine/Render/PC/Private/D3D12CommandContext.h"
#include "Engine/Render/PC/Private/D3D12Fence.h"
#include "Engine/Render/PC/Private/D3D12RenderTarget.h"
#include "Engine/Render/PC/Private/D3D12Resources.h"

void D3D12GraphicsContext::Initialize(D3D12CommandContext* pGraphicCommandContext,
                                      ID3D12CommandAllocator* pCommandAllocator, const D3D12Resource* pStagingBufferPool)
{
    mCommandContext = pGraphicCommandContext;
    mStagingBufferPool = pStagingBufferPool;
    mCommandAllocator = pCommandAllocator;
    mResourceStateTracker.reset(new ResourceStateTracker());
}

std::unique_ptr<RHIConstantBuffer> D3D12GraphicsContext::AllocTempConstantBuffer(uint16_t size)
{
    return std::make_unique<RHIConstantBuffer>(mCommandContext->AllocTemporaryBuffer(size));
}

std::unique_ptr<RHIInstanceBuffer> D3D12GraphicsContext::AllocTempInstanceBuffer(uint32_t size, uint32_t stride)
{
    return std::make_unique<RHIInstanceBuffer>(mCommandContext->AllocTemporaryBuffer(size, stride));
}

void D3D12GraphicsContext::UpdateInstanceBuffer(RHIInstanceBuffer* pBuffer, void* pData, uint64_t offset, uint64_t size)
{
    D3D12UploadBuffer* pUploadBuffer = static_cast<D3D12UploadBuffer*>(pBuffer->GetBuffer());
    pUploadBuffer->Update(pData, offset, size);
}

void D3D12GraphicsContext::UpdateConstantBuffer(RHIConstantBuffer* pBuffer, void* pData, uint64_t offset, uint64_t size)
{
    D3D12UploadBuffer* pUploadBuffer = static_cast<D3D12UploadBuffer*>(pBuffer->GetBuffer());
    pUploadBuffer->Update(pData, offset, size);
}

void D3D12GraphicsContext::UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size, uint64_t dstStart,
                                        uint64_t srcStart)
{
    ASSERT(pDst && pStagingBuffer, TEXT("invalid destination buffer or staging buffer."));
    D3D12Buffer* pNativeDstBuffer = static_cast<D3D12Buffer*>(pDst->GetBuffer());
    ASSERT(pNativeDstBuffer->GetDesc().mResourceType != ResourceType::DYNAMIC, TEXT("violent access: GPU WRITE on upload heap."));
    const D3D12Resource* pDstResource = pNativeDstBuffer->GetD3D12Resource();
    D3D12UploadBuffer* pNativeStagingBuffer = static_cast<D3D12UploadBuffer*>(pStagingBuffer->GetBuffer());
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pDstResource, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyBuffer(mCommandList, pDstResource, mStagingBufferPool, size, dstStart, pNativeStagingBuffer->Offset() + srcStart);
}

void D3D12GraphicsContext::UpdateTexture(RHITextureWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap)
{
    ASSERT(pDst && pStagingBuffer, TEXT("invalid destination texture or staging buffer."));
    const D3D12Resource* pDstResource = static_cast<D3D12Texture*>(pDst->GetBuffer())->GetD3D12Resource();
    D3D12UploadBuffer* pNativeStagingBuffer = static_cast<D3D12UploadBuffer*>(pStagingBuffer->GetBuffer());
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pDstResource, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyTexture(mCommandList, pDstResource,
        mStagingBufferPool,
        TextureCopyLocation::Texture2DLocation(mipmap, 1/* TODO: numMipmaps*/),
        TextureCopyLocation::BufferLocation(pNativeStagingBuffer->Offset()));
}

void D3D12GraphicsContext::CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc, const TextureCopyLocation& dstLocation,
                                     const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth)
{
    ASSERT(pDst && pSrc, TEXT("invalid destination texture or source texture."));
    const D3D12Resource* pNativeSrcTexture = static_cast<D3D12Texture*>(pSrc->GetBuffer())->GetD3D12Resource();
    const D3D12Resource* pNativeDstTexture = static_cast<D3D12Texture*>(pDst->GetBuffer())->GetD3D12Resource();
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pNativeDstTexture, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    barriers = mResourceStateTracker->ConvertResourceState(pNativeSrcTexture, ResourceState::COPY_SOURCE);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyTexture(mCommandList, pNativeDstTexture, pNativeSrcTexture,
        dstLocation, srcLocation, width, height, depth);
}

void D3D12GraphicsContext::ClearRenderTarget(const RHIRenderTarget* pRenderTarget, const Float4& clearColor,
                                           const Rect* clearRects, uint32_t numRects)
{
    const D3D12FrameBuffer* pNativeRenderTarget = static_cast<const D3D12FrameBuffer*>(pRenderTarget->GetBuffer());
    const float color[4] = { clearColor.x, clearColor.y, clearColor.z, clearColor.w };
    mCommandList->ClearRenderTargetView(pNativeRenderTarget->CPUHandle(), color, numRects, reinterpret_cast<const D3D12_RECT*>(clearRects));
}

void D3D12GraphicsContext::ClearDepthStencil(const RHIDepthStencil* pDepthStencil, bool clearDepth, bool clearStencil,
	float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects)
{
    const D3D12FrameBuffer* pNativeDepthStencil = static_cast<const D3D12FrameBuffer*>(pDepthStencil->GetBuffer());
    D3D12_CLEAR_FLAGS flags = (clearDepth ? D3D12_CLEAR_FLAG_DEPTH : static_cast<D3D12_CLEAR_FLAGS>(0)) | (clearStencil ? D3D12_CLEAR_FLAG_STENCIL : static_cast<D3D12_CLEAR_FLAGS>(0));
    mCommandList->ClearDepthStencilView(pNativeDepthStencil->CPUHandle(), flags, depth, stencil, numRects, reinterpret_cast<const RECT*>(clearRects));
}

void D3D12GraphicsContext::SetPipelineState(const PipelineInitializer& initializer)
{
    ID3D12PipelineState* pPSO = mCommandContext->GetPipelineStateObject(mRootSignature, initializer);
    mCommandList->SetPipelineState(pPSO);
}

void D3D12GraphicsContext::SetConstantBuffers(uint8_t baseSlot, uint8_t numSlots, RHIConstantBuffer* pConstants[])
{
    ASSERT(pConstants, TEXT("invalid pConstants"));
    const RootSignatureLayout& layout = mRootSignature->mLayout;
    uint8_t upperSlot = baseSlot + numSlots;
    ASSERT(layout.ValidateBRegister(upperSlot), TEXT("slot out of bound."));
    for (int i = baseSlot; i < layout.mMaterialConstantsBase; ++i)
    {
        D3D12UploadBuffer* pNativeCBuffer = static_cast<D3D12UploadBuffer*>(pConstants[i - baseSlot]->GetBuffer());
        mCommandList->SetGraphicsRootConstantBufferView(baseSlot, pNativeCBuffer->GpuAddress());
    }
    D3D12Device* pDevice = mCommandContext->GetDevice();
    for (int i = layout.mMaterialConstantsBase; i < upperSlot; ++i)
    {
        D3D12UploadBuffer* pNativeCBuffer = static_cast<D3D12UploadBuffer*>(pConstants[i - baseSlot]->GetBuffer());
        pDevice->CreateConstantBufferView(pNativeCBuffer->GetConstantDescView(), mDescriptorHandles[i - layout.mMaterialConstantsBase].mCPUHandle);
    }
}

void D3D12GraphicsContext::SetConstantBuffer(uint8_t slot, RHIConstantBuffer* pConstants)
{
    D3D12UploadBuffer* pNativeCBuffer = static_cast<D3D12UploadBuffer*>(pConstants->GetBuffer());
    if (slot < mRootSignature->mLayout.mMaterialConstantsBase)
    {
        mCommandList->SetGraphicsRootConstantBufferView(slot, pNativeCBuffer->GpuAddress());
    }
    else
    {
        D3D12Device* pDevice = mCommandContext->GetDevice();
        pDevice->CreateConstantBufferView(pNativeCBuffer->GetConstantDescView(), mDescriptorHandles[slot - mRootSignature->mLayout.mMaterialConstantsBase].mCPUHandle);
    }
}

void D3D12GraphicsContext::SetInstanceBuffer(uint8_t slot, RHIInstanceBuffer* pInstanceBuffer)
{
    ASSERT(pInstanceBuffer, TEXT("invalid instance buffer"));
    D3D12UploadBuffer* pNativeBuffer = static_cast<D3D12UploadBuffer*>(pInstanceBuffer->GetBuffer());
    D3D12Device* pDevice = mCommandContext->GetDevice();
    const RHIBufferDesc& desc = pNativeBuffer->GetDesc();
    pDevice->CreateShaderResourceView(pNativeBuffer->GetPoolResource()->D3D12ResourcePtr(),
        {
    		DXGI_FORMAT_UNKNOWN, D3D12_SRV_DIMENSION_BUFFER,
	        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
	        D3D12_BUFFER_SRV{
	            pNativeBuffer->Offset() / desc.mStride, static_cast<uint32_t>(desc.mSize) / desc.mStride, desc.mStride,
	            D3D12_BUFFER_SRV_FLAG_NONE
	        }
        }, mResourceHandles[slot].mCPUHandle);
}

void D3D12GraphicsContext::SetTextures(uint8_t baseSlot, uint8_t numSlots, RHINativeTexture* textures[])
{
    ASSERT(textures, TEXT("invalid textures"));
    uint8_t upper = baseSlot + numSlots;
    ASSERT(upper < mRootSignature->mLayout.mNumTextures, TEXT("slot out of bound."));
    D3D12Device* pDevice = mCommandContext->GetDevice();
    for (int i = baseSlot; i < upper; ++i)
    {
        D3D12Texture& texture = *static_cast<D3D12Texture*>(textures[i - baseSlot]);
        pDevice->CreateShaderResourceView(texture.GetD3D12Resource()->D3D12ResourcePtr(), texture.GetSRVDesc(), mResourceHandles[i].mCPUHandle);
    }
}

void D3D12GraphicsContext::SetTexture(uint8_t slot, RHINativeTexture* textures)
{
    D3D12Device* pDevice = mCommandContext->GetDevice();
    D3D12Texture& texture = *static_cast<D3D12Texture*>(textures);
    pDevice->CreateShaderResourceView(texture.GetD3D12Resource()->D3D12ResourcePtr(), texture.GetSRVDesc(), mResourceHandles[slot].mCPUHandle);
}

void D3D12GraphicsContext::SetViewPorts(Viewport* viewports, uint32_t numViewports)
{
    std::unique_ptr<D3D12_VIEWPORT[]> d3d12Viewports{ new D3D12_VIEWPORT[numViewports] };
    for (uint32_t i = 0; i < numViewports; i++)
    {
        D3D12_VIEWPORT& d3d12Viewport = d3d12Viewports[i];
        d3d12Viewport.TopLeftX = viewports[i].mLeft;
        d3d12Viewport.TopLeftY = viewports[i].mTop;
        d3d12Viewport.Width = viewports[i].mWidth;
        d3d12Viewport.Height = viewports[i].mHeight;
        d3d12Viewport.MinDepth = viewports[i].mMinDepth;
        d3d12Viewport.MaxDepth = viewports[i].mMaxDepth;
    }

    mCommandList->RSSetViewports(numViewports, d3d12Viewports.get());
}

void D3D12GraphicsContext::SetScissorRect(Rect* scissorRects, uint32_t numScissorRects)
{
    std::unique_ptr<RECT[]> d3d12ScissorRects{ new RECT[numScissorRects] };
    for (uint32_t i = 0; i < numScissorRects; i++)
    {
        D3D12_RECT& d3d12ScissorRect = d3d12ScissorRects[i];
        d3d12ScissorRect.top = scissorRects[i].mTop;
        d3d12ScissorRect.left = scissorRects[i].mLeft;
        d3d12ScissorRect.bottom = scissorRects[i].mBottom;
        d3d12ScissorRect.right = scissorRects[i].mRight;
    }
    mCommandList->RSSetScissorRects(numScissorRects, d3d12ScissorRects.get());
}

void D3D12GraphicsContext::SetRenderTargetsAndDepthStencil(RHIRenderTarget* const* renderTargets, uint32_t numRenderTargets,
                                                           RHIDepthStencil const* depthStencilTarget)
{
    ASSERT(numRenderTargets <= 1, TEXT("multi-render target not supported currently."));
    const std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> rtvs{ numRenderTargets ? new D3D12_CPU_DESCRIPTOR_HANDLE[numRenderTargets] : nullptr };
    for (uint32_t i = 0; i < numRenderTargets; i++)
    {
        D3D12FrameBuffer* pNativeRenderTarget = static_cast<D3D12FrameBuffer*>(renderTargets[i]->GetBuffer());
        auto&& barriers = mResourceStateTracker->ConvertResourceState(pNativeRenderTarget->GetD3D12Resource(), ResourceState::RENDER_TARGET);
        if (!barriers.empty())
        {
            mCommandList->ResourceBarrier(barriers.size(), barriers.data());
        }
        rtvs[i] = pNativeRenderTarget->CPUHandle();
    }

    if (!depthStencilTarget)
    {
	    mCommandList->OMSetRenderTargets(numRenderTargets, rtvs.get(), false, nullptr);
        return;
    }

    D3D12FrameBuffer* pNativeDepthStencil = static_cast<D3D12FrameBuffer*>(depthStencilTarget->GetBuffer());
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pNativeDepthStencil->GetD3D12Resource(), ResourceState::DEPTH_WRITE);
    if (!barriers.empty())
    {
	    mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = pNativeDepthStencil->CPUHandle();
    mCommandList->OMSetRenderTargets(numRenderTargets, rtvs.get(), false, &dsv);
}

void D3D12GraphicsContext::SetVertexBuffers(RHIVertexBuffer** vertexBuffers, uint8_t numVertexBuffers)
{
    if (!vertexBuffers)
    {
	    D3D12_VERTEX_BUFFER_VIEW vbv = { 0, 0, 0 };
	    mCommandList->IASetVertexBuffers(0, 1, &vbv);
        return;
    }

    D3D12_VERTEX_BUFFER_VIEW* vbvs = new D3D12_VERTEX_BUFFER_VIEW[numVertexBuffers];
    for (int i = 0; i < numVertexBuffers; ++i)
    {
	    D3D12VertexBuffer* buffer = static_cast<D3D12VertexBuffer*>(vertexBuffers[0]->GetBuffer());
	    vbvs[i] = buffer->GetVertexBufferView();
    }

    mCommandList->IASetVertexBuffers(0, numVertexBuffers, vbvs);
    delete[] vbvs;
}

void D3D12GraphicsContext::SetIndexBuffer(const RHIIndexBuffer* pIndexBuffer)
{
    D3D12IndexBuffer* buffer = static_cast<D3D12IndexBuffer*>(pIndexBuffer->GetBuffer());
    const D3D12_INDEX_BUFFER_VIEW ibv = buffer->GetIndexBufferView();
    mCommandList->IASetIndexBuffer(&ibv);
}

void D3D12GraphicsContext::DrawInstanced(uint32_t verticesPerInstance, uint32_t baseVertex, uint32_t instanceCount, uint32_t baseInstance)
{
    mCommandList->DrawInstanced(verticesPerInstance, instanceCount, baseVertex, baseInstance);
}

void D3D12GraphicsContext::DrawIndexedInstanced(uint32_t indicesPerInstance, uint32_t baseIndex, uint32_t baseVertex, uint32_t instanceCount, uint32_t baseInstance)
{
    mCommandList->DrawIndexedInstanced(indicesPerInstance, instanceCount, baseIndex, static_cast<int32_t>(baseVertex), baseInstance);
}

void D3D12GraphicsContext::InsertFence(RHIFence* pFence, uint64_t semaphore)
{
    if (pFence->GetValue() >= semaphore) return;
    mSynchronizes.emplace_back(static_cast<D3D12Fence*>(pFence), semaphore);
}

void D3D12GraphicsContext::BeginBinding()
{
    mCommandContext->AllocDescriptors(mDescriptorHandles, mResourceHandles, mRootSignature);
    //mDrawCalls.clear();
}

void D3D12GraphicsContext::Reset(ID3D12GraphicsCommandList* pCommandList)
{
    mRootSignature = mCommandContext->GetRootSignature(/*"UniversalRootSignature"*/);
    //mDrawCalls.clear();
    mResourceStateTracker->Cancel();
    mDescriptorHandles.reset();
    mResourceHandles = nullptr;
    mCommandList = pCommandList;

    mCommandAllocator->Reset();
    if (mCommandList)
    {
	    mCommandList->Reset(mCommandAllocator, nullptr);

        mCommandList->SetGraphicsRootSignature(mRootSignature->mRootSignature.Get());
        std::unique_ptr<ID3D12DescriptorHeap* []> pHeaps;
        uint16_t numHeaps;
        mCommandContext->GetOnlineDescriptorHeaps(pHeaps, numHeaps);
        mCommandList->SetDescriptorHeaps(numHeaps, pHeaps.get());
    }
}

void D3D12GraphicsContext::TransitionResource(const D3D12Resource* pResource, ResourceState dstState) const
{
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pResource, dstState);
    if (barriers.empty()) return;
    mCommandList->ResourceBarrier(barriers.size(), barriers.data());
}

void D3D12GraphicsContext::EndBinding()
{
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->SetGraphicsRootDescriptorTable(mRootSignature->mLayout.mMaterialConstantsBase, mDescriptorHandles[0].mGPUHandle);
    //if (mDrawCalls.empty()) return;
    /*mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->SetGraphicsRootDescriptorTable(2, mDescriptorHandles[0].mGPUHandle);*/
	//for (const auto& drawCall : mDrawCalls)
	//{
 //       if (drawCall.mIndexed)
 //       {
	//        mCommandList->DrawIndexedInstanced(drawCall.mNumVerticesPerInstance, drawCall.mNumInstances, drawCall.mBaseIndex, drawCall.mBaseVertex, drawCall.mBaseInstance);
 //       }
 //       else
 //       {
 //           mCommandList->DrawInstanced(drawCall.mNumVerticesPerInstance, drawCall.mNumInstances, drawCall.mBaseVertex, drawCall.mBaseInstance);
 //       }
	//}
}

//void D3D12GraphicsContext::Execute()
//{
//    ID3D12CommandQueue* pQueue = mCommandContext->GetDirectQueue();
//    auto&& barriers = mResourceStateTracker->BuildPreTransitions();
//    if (!barriers.empty())
//    {
//	    ID3D12GraphicsCommandList* =
//    }
//    else
//    {
//	    pQueue->ExecuteCommandLists(1, CommandListCast(&mCommandList));
//    }
//}
//
//void D3D12GraphicsContext::ExecuteWithSync(RHIFence* pFence, uint64_t semaphore)
//{
//    ID3D12CommandQueue* pQueue = mCommandContext->GetDirectQueue();
//
//    pQueue->Signal(mFence->GetD3D12Fence(), mSemaphore);
//}

/*oid D3D12GraphicsContext::Synchronize(RHIFence* pFence, uint64_t semaphore)
{
    mFence = static_cast<D3D12Fence*>(pFence);
    mFenceValue = semaphore;
}*/

void D3D12CopyContext::Initialize(const D3D12Resource* pStagingBufferPool, ID3D12CommandAllocator* pDedicatedAllocator)
{
    mStagingBufferPool = pStagingBufferPool;
    mCommandAllocator = pDedicatedAllocator;
    mResourceStateTracker.reset(new ResourceStateTracker());
}

void D3D12CopyContext::Reset(ID3D12GraphicsCommandList* pCommandList)
{
    mResourceStateTracker->Cancel();
    mSynchronizes.clear();
    mCommandList = pCommandList;
    //mDedicatedFence->InsertFence(mSemaphore);
    mCommandAllocator->Reset();
    if (mCommandList) mCommandList->Reset(mCommandAllocator, nullptr);
}

void D3D12CopyContext::UpdateBuffer(RHIConstantBuffer* pCBuffer, void* pData, uint64_t offset, uint64_t size)
{
	ASSERT(pCBuffer, TEXT("invalid constant buffer."));
    D3D12UploadBuffer* pNativeCBuffer = static_cast<D3D12UploadBuffer*>(pCBuffer->GetBuffer());
    pNativeCBuffer->Update(pData, offset, size);
}

void D3D12CopyContext::UpdateBuffer(RHIBufferWrapper* pDst, RHIStagingBuffer* pStagingBuffer, uint64_t size,
                                    uint64_t dstStart, uint64_t srcStart)
{
    ASSERT(pDst && pStagingBuffer, TEXT("invalid destination buffer or staging buffer."));
    D3D12Buffer* pNativeDstBuffer = static_cast<D3D12Buffer*>(pDst->GetBuffer());
    ASSERT(pNativeDstBuffer->GetDesc().mResourceType != ResourceType::DYNAMIC, TEXT("violent access: GPU WRITE on upload heap."));
    const D3D12Resource* pDstResource = pNativeDstBuffer->GetD3D12Resource();
    D3D12UploadBuffer* pNativeStagingBuffer = static_cast<D3D12UploadBuffer*>(pStagingBuffer->GetBuffer());
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pDstResource, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyBuffer(mCommandList, pDstResource, mStagingBufferPool, size, dstStart, pNativeStagingBuffer->Offset() + srcStart);
}

void D3D12CopyContext::UpdateTexture(RHINativeTexture* pDst, RHIStagingBuffer* pStagingBuffer, uint8_t mipmap)
{
    ASSERT(pDst && pStagingBuffer, TEXT("invalid destination texture or staging buffer."));
    const D3D12Resource* pDstResource = static_cast<D3D12Texture*>(pDst)->GetD3D12Resource();
    D3D12UploadBuffer* pNativeStagingBuffer = static_cast<D3D12UploadBuffer*>(pStagingBuffer->GetBuffer());
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pDstResource, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyTexture(mCommandList, pDstResource,
        mStagingBufferPool,
        TextureCopyLocation::Texture2DLocation(mipmap, 1/* TODO: numMipmaps*/),
        TextureCopyLocation::BufferLocation(pNativeStagingBuffer->Offset()));
}

void D3D12CopyContext::CopyTexture(RHITextureWrapper* pDst, RHITextureWrapper* pSrc,
	const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height,
	uint32_t depth)
{
    ASSERT(pDst && pSrc, TEXT("invalid destination texture or source texture."));
    const D3D12Resource* pNativeSrcTexture = static_cast<D3D12Texture*>(pSrc->GetBuffer())->GetD3D12Resource();
    const D3D12Resource* pNativeDstTexture = static_cast<D3D12Texture*>(pDst->GetBuffer())->GetD3D12Resource();
    auto&& barriers = mResourceStateTracker->ConvertResourceState(pNativeDstTexture, ResourceState::COPY_DEST);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    barriers = mResourceStateTracker->ConvertResourceState(pNativeSrcTexture, ResourceState::COPY_SOURCE);
    if (!barriers.empty())
    {
        mCommandList->ResourceBarrier(barriers.size(), barriers.data());
    }
    D3D12CommandContext::CopyTexture(mCommandList, pNativeDstTexture, pNativeSrcTexture,
                                     dstLocation, srcLocation, width, height, depth);
}

void D3D12CopyContext::InsertFence(RHIFence* pFence, uint64_t semaphore)
{
    if (pFence->GetValue() >= semaphore) return;
    mSynchronizes.emplace_back(static_cast<D3D12Fence*>(pFence), semaphore);
}

// void D3D12GraphicsContext::SubmitWithSynchronize(RHIFence* pFence, uint64_t semaphore)
// {
//     std::vector<D3D12_RESOURCE_BARRIER>&& barriers = mCommandContext->Finalize();
//     mFence->InsertFence(mFenceValue);
//     mRHI->SubmitRenderCommandsWithSync(barriers, &mCommandContext, ((D3D12Fence*)pFence)->GetD3D12Fence(), semaphore);
// }
#endif