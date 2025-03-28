#ifdef WIN32
#include "D3dCommandList.h"

#include "Engine/common/Exception.h"
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/PC/Resource/StaticBuffer.h"

class DynamicBuffer;

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
    return reinterpret_cast<ID3D12Fence*>(D3dObject::nativePtr());
}

Fence::Fence() = default;

Fence::Fence(ID3D12Fence* fence) : D3dObject(fence)
{ }

void D3dCommandList::transition(D3dResource& resource, uint64_t subResourceIndex, ResourceState dstState)
{
    ID3D12Resource* pResource = resource.nativePtr();
    mResourceStateTracker.track(resource);
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(pResource, TEXT("transiting resource is invalid\n"));
#endif
    auto&& conversion = mResourceStateTracker.convertSubResourceState(pResource, subResourceIndex, dstState);
    if (conversion.Transition.pResource)
    {
        nativePtr()->ResourceBarrier(1, &conversion);
    }
}

void D3dCommandList::transition(D3dResource& resource, ResourceState dstState)
{
    ID3D12Resource* pResource = resource.nativePtr();
    mResourceStateTracker.track(resource);
// #if defined(DEBUG) or defined(_DEBUG)
//     ASSERT(pResource, TEXT("transiting resource is invalid\n"));
// #endif
    auto&& conversions = mResourceStateTracker.convertResourceState(pResource, dstState);
    nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
}

void D3dCommandList::drawMeshInstanced() const
{
}

void D3dCommandList::drawMesh(const Mesh& meshData, const DirectX::XMMATRIX& matrix, const Material& material) const
{
}

ID3D12GraphicsCommandList* D3dCommandList::nativePtr() const
{
    return reinterpret_cast<ID3D12GraphicsCommandList*>(D3dObject::nativePtr());
}

D3dCommandList::D3dCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pAllocator) : D3dObject(pCommandList), mCommandAllocator(pAllocator) { }

// update data in the default buffer
// remark: no auto resource state transition
void D3dCommandList::copyResource(StaticBuffer& dst, const D3dResource& src)
{
    transition(dst, ResourceState::COPY_DEST);
    nativePtr()->CopyResource(dst.nativePtr(), src.nativePtr());
}

void D3dCommandList::close() const
{
    nativePtr()->Close();
}

void D3dCommandList::clear()
{
    mResourceStateTracker.cancel();
    nativePtr()->ClearState(nullptr);
}

void D3dCommandList::reset()
{
    // TODO: wait for fence
    nativePtr()->Reset(mCommandAllocator, nullptr);
    mResourceStateTracker.cancel();
}

void D3dCommandList::reset(ID3D12CommandAllocator* pAllocator)
{
    // TODO: 
    nativePtr()->Reset(pAllocator, nullptr);
    mCommandAllocator = pAllocator;
    mResourceStateTracker.cancel();
}

void D3dCommandList::setRenderTargets(const RenderTexture2D* pRenderTarget, uint64_t numRenderTargets, const RenderTexture2D& depthStencil)
{
    // nativePtr()->OMSetRenderTargets()
}

void D3dCommandList::setDepthStencil(RenderTexture2D* pDepthStencil)
{
}

void D3dCommandList::SetShader(const Shader* shader)
{
}

D3dCommandList::~D3dCommandList() = default;

D3dCommandList::D3dCommandList() = default;
#endif
