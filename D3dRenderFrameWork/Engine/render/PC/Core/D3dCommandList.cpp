#ifdef WIN32
#include "D3dCommandList.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/PC/Resource/StaticBuffer.h"

class DynamicHeap;

void Fence::wait(uint64_t fencePoint) const
{
    if (Fence::nativePtr()->GetCompletedValue() < fencePoint)
    {
        HANDLE he = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ThrowIfFailed(
            Fence::nativePtr()->SetEventOnCompletion(fencePoint, he)
        )
        WaitForSingleObject(he, INFINITE);
        CloseHandle(he);
    }
}

ID3D12Fence* Fence::nativePtr() const
{
    return reinterpret_cast<ID3D12Fence*>(D3D12DeviceChild::nativePtr());
}

Fence::Fence() = default;

Fence::Fence(ID3D12Fence* fence) : D3D12DeviceChild(fence)
{ }

void D3D12CommandList::transition(D3dResource& resource, uint64_t subResourceIndex, ResourceState dstState)
{
    ID3D12Resource* pResource = resource.nativePtr();
    mResourceStateTracker.Track(resource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(pResource, TEXT("transiting resource is invalid\n"));
#endif
    auto&& conversion = mResourceStateTracker.ConvertSubResourceState(pResource, subResourceIndex, dstState);
    if (conversion.Transition.pResource)
    {
        nativePtr()->ResourceBarrier(1, &conversion);
    }
}

void D3D12CommandList::transition(D3dResource& resource, ResourceState dstState)
{
    ID3D12Resource* pResource = resource.nativePtr();
    mResourceStateTracker.Track(resource);
// #if defined(DEBUG) or defined(_DEBUG)
//     ASSERT(pResource, TEXT("transiting resource is invalid\n"));
// #endif
    auto&& conversions = mResourceStateTracker.ConvertResourceState(pResource, dstState);
    nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
}

void D3D12CommandList::drawMeshInstanced() const
{
}

void D3D12CommandList::drawMesh(const Mesh& meshData, const DirectX::XMMATRIX& matrix, const MaterialInstance& material) const
{
}

ID3D12GraphicsCommandList* D3D12CommandList::nativePtr() const
{
    return reinterpret_cast<ID3D12GraphicsCommandList*>(D3D12DeviceChild::nativePtr());
}

D3D12CommandList::D3D12CommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pAllocator) : D3D12DeviceChild(pCommandList), mCommandAllocator(pAllocator) { }

// update data in the default buffer
// remark: no auto resource state transition
void D3D12CommandList::copyResource(StaticHeap& dst, const D3dResource& src)
{
    transition(dst, ResourceState::COPY_DEST);
    nativePtr()->CopyResource(dst.nativePtr(), src.nativePtr());
}

void D3D12CommandList::close() const
{
    nativePtr()->Close();
}

void D3D12CommandList::clear()
{
    mResourceStateTracker.Cancel();
    nativePtr()->ClearState(nullptr);
}

void D3D12CommandList::reset()
{
    // TODO: wait for fence
    nativePtr()->Reset(mCommandAllocator, nullptr);
    mResourceStateTracker.Cancel();
}

void D3D12CommandList::reset(ID3D12CommandAllocator* pAllocator)
{
    // TODO: 
    nativePtr()->Reset(pAllocator, nullptr);
    mCommandAllocator = pAllocator;
    mResourceStateTracker.Cancel();
}

void D3D12CommandList::setRenderTargets(const D3dResource* pRenderTarget, uint64_t numRenderTargets, const D3dResource& depthStencil)
{
    // nativePtr()->OMSetRenderTargets()
    auto* nativeCmdList = nativePtr();
    nativeCmdList->OMSetRenderTargets(1, &hRTV, false, &hDSV);
    nativeCmdList->ClearRenderTargetView(hRTV, DirectX::Colors::LightSteelBlue, 0, nullptr);
    nativeCmdList->ClearDepthStencilView(hDSV, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}

void D3D12CommandList::setDepthStencil(RenderTexture2D* pDepthStencil)
{
}

void D3D12CommandList::SetShader(const HlslShader* shader)
{
}

D3D12CommandList::~D3D12CommandList() = default;

D3D12CommandList::D3D12CommandList() = default;
#endif
