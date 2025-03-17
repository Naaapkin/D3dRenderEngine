#ifdef WIN32
#include <Engine/common/PC/WException.h>
#include <Engine/render/PC/D3dCommandList.h>
#include <Engine/render/PC/D3dCommandQueue.h>
#include <Engine/render/PC/D3dGraphicContext.h>
#include <Engine/render/PC/D3dUtil.h>
#include <Engine/render/PC/RenderContext.h>
#include <Engine/render/PC/RenderResource/DefaultHeap.h>

bool Fence::IsValid() const
{
    return mFence != nullptr; 
}

void Fence::Wait() const
{
    if (mFence->GetCompletedValue() < mFenceValue)
    {
        HANDLE he = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ThrowIfFailed(
            mFence->SetEventOnCompletion(mFenceValue, he)
        )
        WaitForSingleObject(he, INFINITE);
        CloseHandle(he);
    }
}

Fence::Fence() = default;

Fence::Fence(ID3D12Fence* fence) : mFence(fence), mFenceValue()
{ }

void D3dCommandList::FlushCommandList(RenderContext& renderContext)
{
	mRenderContext = &renderContext;
    ThrowIfFailed(mCommandList->Close());
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator, nullptr));
}

void D3dCommandList::TransitSubResource(D3dResource& resource, uint64_t subResourceIndex,
                                        ResourceState stateAfter) const
{
    if (subResourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
        TransitionAll(resource, stateAfter);
    else
        TransitionSingle(resource, subResourceIndex, stateAfter);
}

void D3dCommandList::TransitResource(D3dResource& resource, ResourceState stateAfter) const
{
    ID3D12Resource* pResource = resource.NativePtr();
#ifdef _DEBUG | DEBUG
    ASSERT(pResource, TEXT("transiting resource is invalid\n"));
#endif
    D3D12_RESOURCE_DESC&& desc = pResource->GetDesc();
    auto* transition = mRenderContext->GetResourceStates(&resource);
#ifdef DEBUG || _DEBUG
    ASSERT(transition, TEXT("resource not declared as used\n"));
#endif
    uint32_t stateAfterU = static_cast<uint32_t>(stateAfter);
    ResourceState& stateBefore = transition[1];
    if (stateBefore == ResourceState::UNKNOWN)
    {
        transition[0] = stateAfter;
        stateBefore = stateAfter;
        return;
    }

    // check if the explicit barrier is needed
    bool isBufferOrSimultaneous = desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS ||
        desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
    if (!::CanImplicitTransit(
        static_cast<D3D12_RESOURCE_STATES>(stateBefore),
        stateAfterU,
        isBufferOrSimultaneous))
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
                                                                              static_cast<D3D12_RESOURCE_STATES>(
                                                                                  stateBefore),
                                                                              static_cast<D3D12_RESOURCE_STATES>(
                                                                                  stateAfterU));
        mCommandList->ResourceBarrier(1, &barrier);
    }
    stateBefore = static_cast<ResourceState>(stateAfterU);
}

void D3dCommandList::TransitionSubResource(D3dResource& resource, uint64_t subResourceIndex, ResourceState stateAfter,
                                           bool begin) const
{
}

void D3dCommandList::TransitionResource(D3dResource& resource, ResourceState stateAfter, bool begin) const
{
}

void D3dCommandList::DrawMeshInstanced() const
{
}

void D3dCommandList::DrawMesh() const
{
}

void D3dCommandList::TransitionSingle(D3dResource& resource, uint64_t subResourceIndex,
                                      ResourceState stateAfter) const
{
    ID3D12Resource* pResource = resource.NativePtr();
    uint64_t count = resource.SubResourceCount();
#ifdef DEBUG | _ DEBUG
    ASSERT(pResource, TEXT("transiting resource is invalid\n"));
#endif
    D3D12_RESOURCE_DESC&& desc = pResource->GetDesc();
    bool isBufferOrSimultaneous = desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS ||
                desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
    auto* transition = mRenderContext->GetSubResourceTransition(&resource, subResourceIndex);
#ifdef _DEBUG | DEBUG
    ASSERT(count > subResourceIndex, TEXT("subResource index is out of bound\n"))
    ASSERT(transition, TEXT("resource not declared as used\n"));
#endif
    
    uint32_t stateAfterU = static_cast<uint32_t>(stateAfter);
    ResourceState& stateBefore = transition[1];
    if (stateBefore == ResourceState::UNKNOWN)
    {
        transition[0] = stateAfter;
        stateBefore = stateAfter;
        return;
    }
    if (!::CanImplicitTransit(
        static_cast<D3D12_RESOURCE_STATES>(stateBefore),
        stateAfterU,
        isBufferOrSimultaneous))
    {
        // if the explicit barrier is needed, insert the barrier.
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.NativePtr(),
        static_cast<D3D12_RESOURCE_STATES>(stateBefore),
        static_cast<D3D12_RESOURCE_STATES>(stateAfterU),
        subResourceIndex);
        mCommandList->ResourceBarrier(1, &barrier);
    }
    // transition[1] = stateAfter
    stateBefore = static_cast<ResourceState>(stateAfterU);
}

void D3dCommandList::TransitionAll(D3dResource& resource,
    ResourceState stateAfter) const
{
    ID3D12Resource* pResource = resource.NativePtr();
    D3D12_RESOURCE_DESC&& desc = pResource->GetDesc();
    bool isBufferOrSimultaneous = desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS ||
                desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
#ifdef _DEBUG | DEBUG
    ASSERT(pResource, TEXT("transiting resource is invalid\n"))
#endif
    uint64_t count = resource.SubResourceCount();
    uint64_t explicitTransitionCount = 0;
    D3D12_RESOURCE_BARRIER* barriers = new D3D12_RESOURCE_BARRIER[count];
    for (uint64_t i = 0; i < count; ++i)
    {
        auto* transition = mRenderContext->GetSubResourceTransition(&resource, i);
#ifdef DEBUG | _DEBUG
        ASSERT(transition, TEXT("resource not declared as used\n"));
#endif
        uint32_t stateAfterU = static_cast<uint32_t>(stateAfter);
        ResourceState& stateBefore = transition[1];
        if (stateBefore == ResourceState::UNKNOWN)
        {
            transition[0] = stateAfter;
            stateBefore = stateAfter;
            return;
        }
        // check if the explicit barrier is needed
        if (!::CanImplicitTransit(
            static_cast<D3D12_RESOURCE_STATES>(stateBefore),
            stateAfterU,
            isBufferOrSimultaneous))
        {
            barriers[explicitTransitionCount] = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
                static_cast<D3D12_RESOURCE_STATES>(stateBefore),
                static_cast<D3D12_RESOURCE_STATES>(stateAfterU),
                i);
            explicitTransitionCount++;
        }
        stateBefore = static_cast<ResourceState>(stateAfterU);
    }
    
	mCommandList->ResourceBarrier(explicitTransitionCount, barriers);
    
    delete[] barriers;
}

void D3dCommandList::ResolveFirstTransitions() const
{
#ifdef _DEBUG | DEBUG
    ASSERT(mRenderContext, TEXT("commandList should be bound to a renderContext before being used"))
#endif
    ID3D12GraphicsCommandList* pPreTransition = nullptr;
    ThrowIfFailed(::gGraphicContext()->DeviceHandle()->CreateCommandList(
        0, mRenderContext->GetCommandQueue()->GetQueueType(), mCommandAllocator,
        nullptr, IID_PPV_ARGS(&pPreTransition))
        );
    
    auto transitionAll = [pPreTransition](D3dResource& resource, ResourceState* transition){
        ID3D12Resource* pResource = resource.NativePtr();
        D3D12_RESOURCE_DESC&& desc = pResource->GetDesc();
#ifdef _DEBUG | DEBUG
        ASSERT(pResource, TEXT("transiting resource is invalid\n"))
#endif
        bool isBufferOrSimultaneous = desc.Flags & D3D12_RESOURCE_FLAG_NONE | desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
        
        if (isBufferOrSimultaneous)
        {
            // if the resource is buffer or simultaneous, it will always decay to COMMON state after execution,
            // so we can just use COMMON state as 'stateBefore'.
            auto stateBefore = D3D12_RESOURCE_STATE_COMMON;
            uint32_t stateAfterU = static_cast<uint32_t>(transition[0]);
            if (transition[0] != ResourceState::UNKNOWN &&  // skip the situation that no transitions happened on this resource.
                !::CanImplicitTransit(stateBefore, stateAfterU, isBufferOrSimultaneous))
            {
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
                   stateBefore,
                   static_cast<D3D12_RESOURCE_STATES>(stateAfterU));
                pPreTransition->ResourceBarrier(1, &barrier);
            }
        }
        else
        {
            // if subresources are in different states, they may promote to different states, so
            // we just insert a explicit barrier to make sure they transition to the same state.
            uint64_t count = resource.SubResourceCount();
            auto* resourceStates = resource.ResourceStates();
            uint64_t explicitTransitionCount = 0;
            D3D12_RESOURCE_BARRIER* barriers = new D3D12_RESOURCE_BARRIER[count];
            for (uint64_t i = 0; i < count; ++i)
            {
                if (transition[0] != ResourceState::UNKNOWN)
                {
                    barriers[explicitTransitionCount] = CD3DX12_RESOURCE_BARRIER::Transition(pResource,
                       static_cast<D3D12_RESOURCE_STATES>(resourceStates[i]),
                       static_cast<D3D12_RESOURCE_STATES>(transition[0]),
                       i);
                    explicitTransitionCount++;
                }
            }
            pPreTransition->ResourceBarrier(explicitTransitionCount, barriers);
            delete[] barriers;
        }
    };
    auto transitionSingle = [pPreTransition](D3dResource& resource, uint64_t subResourceIndex, ResourceState* transition)
    {
        ID3D12Resource* pResource = resource.NativePtr();
        D3D12_RESOURCE_DESC&& desc = pResource->GetDesc();
        bool isBufferOrSimultaneous = desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS ||
                    desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER;
    
#ifdef _DEBUG | DEBUG
        ASSERT(pResource, TEXT("transiting resource is invalid\n"))
        ASSERT(resource.SubResourceCount() > subResourceIndex, TEXT("subResource index is out of bound\n"))
#endif
    
        uint32_t stateAfterU = static_cast<uint32_t>(transition[0]);
        ResourceState& stateBefore = resource.ResourceStates()[subResourceIndex];
        // check if the explicit barrier is needed
        if (transition[0] != ResourceState::UNKNOWN &&
            ::CanImplicitTransit(static_cast<D3D12_RESOURCE_STATES>(stateBefore),
            stateAfterU, isBufferOrSimultaneous))
        {
            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.NativePtr(),
                static_cast<D3D12_RESOURCE_STATES>(stateBefore),
                static_cast<D3D12_RESOURCE_STATES>(stateAfterU),
                subResourceIndex);
            pPreTransition->ResourceBarrier(1, &barrier);
        }
    };
    
    mRenderContext->ForeachResource(transitionAll, transitionSingle);
}

// update data in the default buffer
// remark: no auto resource state transition
void D3dCommandList::UpdateBufferResource(
    DefaultHeap& buffer,
    const byte* data, uint64_t width) const
{
    ID3D12Device* pDevice= gGraphicContext()->DeviceHandle();
    D3D12_RESOURCE_DESC heapDesc = buffer->NativePtr()->GetDesc();
    D3D12_HEAP_PROPERTIES heapProp;
    buffer->NativePtr()->GetHeapProperties(&heapProp, nullptr);
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3dResource uploadBuffer = ::CreateD3dResource(pDevice, D3D12_HEAP_FLAG_NONE, heapProp,
                                                   heapDesc, D3D12_RESOURCE_STATE_GENERIC_READ);
    CD3DX12_RANGE readRange(0, 0);
    void* mappedData = nullptr;
    uploadBuffer.NativePtr()->Map(0, &readRange, &mappedData);
    memcpy(mappedData, data, width);
    uploadBuffer.NativePtr()->Unmap(0, &readRange);

    mCommandList->CopyResource(buffer->NativePtr(), uploadBuffer.NativePtr());
}

void D3dCommandList::Close() const
{
    ThrowIfFailed(mCommandList->Close());
}

ID3D12CommandList* D3dCommandList::NativePtr() const
{
    return mCommandList.Get();
}

D3dCommandList::D3dCommandList() = default;

D3dCommandList::D3dCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pCommandAllocator) :
    mCommandAllocator(pCommandAllocator),
    mRenderContext(nullptr),
    mCommandList(pCommandList) { }

D3dCommandList CreateD3dCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ID3D12GraphicsCommandList* pCommandList;
    ThrowIfFailed(
        pDevice->CreateCommandList(0, type, pCommandAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));
    return { pCommandList, pCommandAllocator };
}

D3dCommandList::~D3dCommandList() = default;
#endif