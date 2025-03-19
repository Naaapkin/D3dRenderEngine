#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/RenderResource/DynamicBuffer.h>

struct RenderContext;
class D3dCommandQueue;
class StaticBuffer;
class D3dGraphicContext;

struct Fence
{
    friend Fence D3dRenderer::sCreateFence();
    friend class D3dCommandList;
    
public:
    void Wait() const;
    // didnt guarantee gpu has reached the fence point, maybe we should wait for it
    ~Fence() = default;

    DELETE_COPY_CONSTRUCTOR(Fence)
    DELETE_COPY_OPERATOR(Fence)
    DEFAULT_MOVE_CONSTRUCTOR(Fence)
    DEFAULT_MOVE_OPERATOR(Fence)
    
private:
    bool IsValid() const;
    Fence();
    Fence(ID3D12Fence* fence);

    ComPtr<ID3D12Fence> mFence;
    uint64_t mFenceValue;
};

class D3dCommandList
{
    friend D3dCommandList CreateD3dCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator, D3D12_COMMAND_LIST_TYPE type);
    
public:
    void FlushCommandList(RenderContext& renderContext);
    void TransitSubResource(D3dResource& resource, uint64_t subResourceIndex, ResourceState stateAfter) const;
    void TransitResource(D3dResource& resource, ResourceState stateAfter) const;
    void TransitionSubResource(D3dResource& resource, uint64_t subResourceIndex, ResourceState stateAfter, bool begin) const;
    void TransitionResource(D3dResource& resource, ResourceState stateAfter, bool begin) const;
    void UpdateBufferResource(const StaticBuffer& buffer, const byte* data, uint64_t width) const;
    void DrawMeshInstanced() const;
    void DrawMesh() const;
    void Close() const;
    ID3D12CommandList* NativePtr() const;
    ~D3dCommandList();
    
    DELETE_COPY_CONSTRUCTOR(D3dCommandList)
    DELETE_COPY_OPERATOR(D3dCommandList)
    DEFAULT_MOVE_CONSTRUCTOR(D3dCommandList)
    DEFAULT_MOVE_OPERATOR(D3dCommandList)
    
private:
    void TransitionSingle(D3dResource& resource, uint64_t subResourceIndex, ResourceState stateAfter) const;
    void TransitionAll(D3dResource& resource, ResourceState stateAfter) const;
    void ResolveFirstTransitions() const;
    D3dCommandList();
    D3dCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pCommandAllocator);

    // we dont manage the commandQueue and commandAllocator objects, just keep a reference to them
    ID3D12CommandAllocator* mCommandAllocator;
    RenderContext* mRenderContext;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
};

D3dCommandList CreateD3dCommandList(ID3D12Device* pDevice, ID3D12CommandAllocator* pCommandAllocator, D3D12_COMMAND_LIST_TYPE type);
#endif
