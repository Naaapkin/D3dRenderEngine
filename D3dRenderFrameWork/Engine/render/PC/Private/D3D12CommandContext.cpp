#ifdef WIN32
#include "D3D12CommandPacker.h"

#include "Engine/render/PC/Native/D3D12PipelineStateManager.h"
#include "Engine/common/Exception.h"
#include "Engine/memory/LinearAllocator.h"
#include "Engine/render/PC/Private/D3D12Command.h"
#include "Engine/render/PC/Resource/D3D12Resources.h"
#include "Engine/render/PC/Native/D3D12DescriptorManager.h"

void D3D12CommandContext::Reset()
{
    mCommandsBuffer.resize(1);
    mAllocator.Initialize(1024, 8, 1024);
    mResourceStateTracker.Cancel();
}

void D3D12CommandContext::TransitionResource(RHIResource* pResource, uint32_t subResource, ResourceState state)
{
    D3D12Resource* pD3D12Resource = reinterpret_cast<D3D12Resource*>(pResource);
    std::pair<bool, D3D12_RESOURCE_BARRIER>&& result = mResourceStateTracker.ConvertSubResourceState(pD3D12Resource, subResource, state);
    if (result.first)
    {
        LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandResourceBarrier>();
        D3D12CommandResourceBarrier* pCommand = new (hCommand.Get()) D3D12CommandResourceBarrier{};
        pCommand->SetBarriers(&result.second, 1);
        mCommandsBuffer.emplace_back(hCommand);
    }
}

void D3D12CommandContext::TransitionResource(RHIResource* pResource, ResourceState state)
{
    D3D12Resource* pD3D12Resource = reinterpret_cast<D3D12Resource*>(pResource);
    std::vector<D3D12_RESOURCE_BARRIER>&& barriers = mResourceStateTracker.ConvertResourceState(pD3D12Resource, state);
    if (!barriers.empty())
    {
        LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandResourceBarrier>();
        D3D12CommandResourceBarrier* pCommand = new (hCommand.Get()) D3D12CommandResourceBarrier{};
        pCommand->SetBarriers(barriers.data(), barriers.size());
        mCommandsBuffer.emplace_back(hCommand);
    }
}

void D3D12CommandContext::SetRootSignatureParas(
    uint8_t materialConstantPara, uint8_t materialTexturePara)
{
    mMaterialCBufferPara = materialConstantPara;
    mMaterialTexturePara = materialTexturePara;
}

void D3D12CommandContext::InsertNativeCommand(const std::function<void(D3D12CommandNative* pCommand)>& injection)
{
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandNative>();
    D3D12CommandNative* pNativeCommand = new (hCommand.Get()) D3D12CommandNative{};
    injection(pNativeCommand);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::SetViewPorts(Viewport* viewports, uint32_t numViewports)
{
    std::vector<D3D12_VIEWPORT> d3d12Viewports{numViewports};
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

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandSetViewports>();
    D3D12CommandSetViewports* pCommand = new (hCommand.Get()) D3D12CommandSetViewports{};
    pCommand->SetViewport(d3d12Viewports.data(), d3d12Viewports.size());
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::SetScissorRect(Rect* scissorRects, uint32_t numScissorRects)
{
    std::vector<RECT> d3d12ScissorRects{numScissorRects};
    for (uint32_t i = 0; i < numScissorRects; i++)
    {
        D3D12_RECT& d3d12ScissorRect = d3d12ScissorRects[i];
        d3d12ScissorRect.top = scissorRects[i].mTop;
        d3d12ScissorRect.left = scissorRects[i].mLeft;
        d3d12ScissorRect.bottom = scissorRects[i].mBottom;
        d3d12ScissorRect.right = scissorRects[i].mRight;
    }
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandSetScissors>();
    D3D12CommandSetScissors* pCommand = new (hCommand.Get()) D3D12CommandSetScissors{};
    pCommand->SetScissorRects(d3d12ScissorRects.data(), numScissorRects);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::SetRenderTargetsAndDepthStencil(RHIFrameBuffer** renderTargets, uint32_t numRenderTargets,
                                                          RHIFrameBuffer* depthStencilTarget)
{
    ASSERT(renderTargets && depthStencilTarget, TEXT("invalid render targets or depth stencil target"));
    
    D3D12FrameBuffer** d3d12FrameBuffers = reinterpret_cast<D3D12FrameBuffer**>(renderTargets);
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs{numRenderTargets};
    for (uint32_t i = 0; i < numRenderTargets; i++)
    {
        rtvs[i] = d3d12FrameBuffers[i]->DescriptorHandle();
        TransitionResource(d3d12FrameBuffers[i]->GetAttachment(), ResourceState::RENDER_TARGET);
    }
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = static_cast<D3D12FrameBuffer*>(depthStencilTarget)->DescriptorHandle();
    TransitionResource(depthStencilTarget->GetAttachment(), ResourceState::DEPTH_WRITE);

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandSetRTDS>();
    D3D12CommandSetRTDS* pCommand = new (hCommand.Get()) D3D12CommandSetRTDS{};
    pCommand->SetRenderTargets(rtvs.data(), rtvs.size());
    pCommand->SetDepthStencilTarget(&dsv);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::ClearRenderTarget(RHIFrameBuffer* pRenderTarget, Float4 clearColor, const Rect* clearRects,
                                           uint32_t numRects)
{
    TransitionResource(pRenderTarget->GetAttachment(), ResourceState::RENDER_TARGET);
    float color[4] = {clearColor.x, clearColor.y, clearColor.z, clearColor.w};
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = static_cast<D3D12FrameBuffer*>(pRenderTarget)->DescriptorHandle();
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandClearRenderTarget>();
    D3D12CommandClearRenderTarget* pCommand = new (hCommand.Get()) D3D12CommandClearRenderTarget{};
    pCommand->SetClearDescriptors(rtv, color, reinterpret_cast<const RECT*>(clearRects), numRects);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::ClearDepthStencil(RHIFrameBuffer* depthStencil, bool clearDepth, bool clearStencil,
                                            float depth, uint32_t stencil, const Rect* clearRects, uint32_t numRects)
{
    TransitionResource(depthStencil->GetAttachment(), ResourceState::DEPTH_WRITE);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = static_cast<D3D12FrameBuffer*>(depthStencil)->DescriptorHandle();
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandClearDepthStencil>();
    D3D12CommandClearDepthStencil* pCommand = new (hCommand.Get()) D3D12CommandClearDepthStencil{};
    D3D12_CLEAR_FLAGS flags = clearDepth ? D3D12_CLEAR_FLAG_DEPTH : static_cast<D3D12_CLEAR_FLAGS>(0) | clearStencil ? D3D12_CLEAR_FLAG_STENCIL : static_cast<D3D12_CLEAR_FLAGS>(0);
    pCommand->SetClearDescriptors(dsv, flags, reinterpret_cast<const RECT*>(clearRects), numRects, depth, stencil);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::BindVertexBuffers(RHIVertexBuffer* pVertexBuffer)
{
    D3D12Buffer* pBuffer = static_cast<D3D12Buffer*>(pVertexBuffer->GetVertexBuffer());
    ID3D12Resource* pResource = static_cast<ID3D12Resource*>(pBuffer->NativeResource());

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandBindVertexBuffer>();
    D3D12CommandBindVertexBuffer* pCommand = new (hCommand.Get()) D3D12CommandBindVertexBuffer{};
    pCommand->SetVertexBuffers({
        pResource->GetGPUVirtualAddress(), static_cast<uint32_t>(pBuffer->GetSize()), pVertexBuffer->GetVertexSize()
    });
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::BindIndexBuffer(RHIIndexBuffer* pIndexBuffer)
{
    D3D12Buffer* pBuffer = static_cast<D3D12Buffer*>(pIndexBuffer->GetIndexBuffer());
    ID3D12Resource* pResource = static_cast<ID3D12Resource*>(pBuffer->NativeResource());
    D3D12_INDEX_BUFFER_VIEW ibv{pResource->GetGPUVirtualAddress(), static_cast<uint32_t>(pBuffer->GetSize()), DXGI_FORMAT_R32_UINT};

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandBindIndexBuffer>();
    D3D12CommandBindIndexBuffer* pCommand = new (hCommand.Get()) D3D12CommandBindIndexBuffer{};
    pCommand->SetIndexBuffer(&ibv);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::SetPipelineState(const PipelineInitializer& pPipelineInitializer)
{
    // TODO: verify pipeline state like count of render targets.
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandSetPipelineState>();
    D3D12CommandSetPipelineState* pCommand = new (hCommand.Get()) D3D12CommandSetPipelineState{};
    pCommand->SetPipelineState(Singleton<D3D12PipelineStateManager>::GetInstance().GetOrCreatePSO(pPipelineInitializer));
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::DrawIndexedInstanced(uint32_t indexPerInstance, uint32_t startIndexLocation, uint32_t instanceCount)
{
    // resource binding
    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandNative>();
    D3D12CommandNative* pNativeCommand = new (hCommand.Get()) D3D12CommandNative{};
    D3D12OnlineDescriptorManager<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>& descriptorManager = Singleton<D3D12OnlineDescriptorManager<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>>::GetInstance(); 
    
    D3D12_GPU_DESCRIPTOR_HANDLE cbufferHandle = descriptorManager.Allocate(DescriptorType::PER_DRAW_CALL);
    D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = descriptorManager.Offset(cbufferHandle, 1);
    uint8_t materialCBufferPara = mMaterialCBufferPara;
    uint8_t materialTexturePara = mMaterialTexturePara;
        
    pNativeCommand->SetBind([=](ID3D12GraphicsCommandList* pCommandList)->void
    {
        pCommandList->SetGraphicsRootDescriptorTable(materialCBufferPara, cbufferHandle);
        pCommandList->SetGraphicsRootDescriptorTable(materialTexturePara, textureHandle);
    });
    mCommandsBuffer.emplace_back(hCommand);

    // draw call
    hCommand = mAllocator.Allocate<RHICommand, D3D12CommandDrawIndexedInstanced>();
    D3D12CommandDrawIndexedInstanced* pCommand = new (hCommand.Get()) D3D12CommandDrawIndexedInstanced{};
    pCommand->SetSubMesh(indexPerInstance, startIndexLocation, instanceCount);
    mCommandsBuffer.emplace_back(hCommand);
}

std::vector<LinearAllocator::Handle<RHICommand>> D3D12CommandContext::Finalize(CommandListType type)
{
    std::vector<D3D12_RESOURCE_BARRIER> barriers = mResourceStateTracker.BuildPreTransitions();
    if (!barriers.empty())
    {
        LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyTextureRegion>();
        D3D12CommandResourceBarrier* pCommand = new(hCommand.Get()) D3D12CommandResourceBarrier{};
        pCommand->SetBarriers(barriers.data(), barriers.size());
        mCommandsBuffer[0] = hCommand;
    }
    mResourceStateTracker.StopTracking(type == CommandListType::COPY);
    return mCommandsBuffer;
}

void D3D12CommandContext::CopyResource(RHIResource* pDst, RHIResource* pSrc)
{
    TransitionResource(pDst, ResourceState::COPY_DEST);
    TransitionResource(pSrc, ResourceState::COPY_SOURCE);

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyResource>();
    D3D12CommandCopyResource* pCommand = new (hCommand.Get()) D3D12CommandCopyResource{};
    pCommand->SetCopyParameters(reinterpret_cast<D3D12Resource*>(pDst)->D3D12ResourcePtr(), reinterpret_cast<D3D12Resource*>(pSrc)->D3D12ResourcePtr());
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::CopyBuffer(RHIBuffer* pDst, RHIBuffer* pSrc, uint64_t size, uint64_t dstStart,
                                     uint64_t srcStart)
{
    TransitionResource(pDst, ResourceState::COPY_DEST);
    TransitionResource(pSrc, ResourceState::COPY_SOURCE);

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyBufferRegion>();
    D3D12CommandCopyBufferRegion* pCommand = new (hCommand.Get()) D3D12CommandCopyBufferRegion{};
    pCommand->SetCopyParameters(static_cast<D3D12Buffer*>(pDst)->D3D12ResourcePtr(), static_cast<D3D12Buffer*>(pSrc)->D3D12ResourcePtr(), size, dstStart, srcStart);
    mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::CopyTexture(RHITexture* pDst, RHIResource* pSrc, uint32_t dstX, uint32_t dstY, uint32_t dstZ, const TextureCopyLocation& location)
{
    ID3D12Device* pDevice = Device()->GetD3D12Device();
    ID3D12Resource* pDstResource = static_cast<D3D12Texture*>(pDst)->D3D12ResourcePtr();
    ID3D12Resource* pSrcResource = static_cast<D3D12Texture*>(pSrc)->D3D12ResourcePtr();
    D3D12_RESOURCE_DESC dstDesc = pDstResource->GetDesc();
    D3D12_RESOURCE_DESC srcDesc = pSrcResource->GetDesc();
    uint64_t size;
    uint32_t subResource = location.mMipmap;
    subResource += dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D || dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ?
             0 : location.mArrayIndex * dstDesc.MipLevels;
    D3D12_TEXTURE_COPY_LOCATION dstLocation;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = subResource;
    dstLocation.pResource = pDstResource;
    pDevice->GetCopyableFootprints(&dstDesc, subResource, 1, 0, &dstLocation.PlacedFootprint, nullptr, nullptr, &size);
    D3D12_TEXTURE_COPY_LOCATION srcLocation;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = subResource;
    srcLocation.pResource = pSrcResource;
    pDevice->GetCopyableFootprints(&srcDesc, 0, 1, 0, &srcLocation.PlacedFootprint, nullptr, nullptr, nullptr);
    D3D12_BOX srcBox { location.mPosX, location.mPosY, location.mPosZ, location.mPosX + location.mWidth, location.mPosY + location.mHeight, location.mPosZ + location.mDepth };

    TransitionResource(pDst, subResource, ResourceState::COPY_DEST);
    TransitionResource(pSrc, subResource, ResourceState::COPY_SOURCE);

    LinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyTextureRegion>();
    D3D12CommandCopyTextureRegion* pCommand = new (hCommand.Get()) D3D12CommandCopyTextureRegion{};
    pCommand->SetCopyParameters(pSrcResource, pDstResource, srcBox, srcLocation, dstLocation, dstX, dstY, dstZ);
    mCommandsBuffer.emplace_back(hCommand);
}

D3D12CommandContext::D3D12CommandContext()
{
    D3D12CommandContext::Reset();
}

D3D12CommandContext::D3D12CommandContext(D3D12Device* parent) : D3D12DeviceChild(parent)
{
    D3D12CommandContext::Reset();
}
#endif