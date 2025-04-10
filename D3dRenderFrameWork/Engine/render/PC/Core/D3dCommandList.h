#pragma once
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/D3dObject.h"
#include "Engine/render/PC/Core/RenderContext.h"
#include "Engine/render/PC/Core/ResourceStateTracker.h"

class MaterialInstance;
struct Mesh;
class D3D12RHIFactory;
class StaticHeap;
enum class ResourceState : uint32_t;
class D3dResource;

class Fence : public D3D12DeviceChild
{
public:
    void wait(uint64_t fencePoint) const;
    ID3D12Fence* nativePtr() const override;
    // didn't guarantee gpu has reached the fence point, maybe we should wait for it
    Fence();
    Fence(ID3D12Fence* fence);
    ~Fence() override = default;

    DELETE_COPY_CONSTRUCTOR(Fence)
    DELETE_COPY_OPERATOR(Fence)
    DEFAULT_MOVE_CONSTRUCTOR(Fence)
    DEFAULT_MOVE_OPERATOR(Fence)
};

class D3D12CommandList final : D3D12DeviceChild
{
    friend void RenderContext::executeCommandLists(const std::vector<D3D12CommandList*>& commandLists) const;
    friend void RenderContext::executeCommandList(D3D12CommandList* commandList) const;
    
public:
    void close() const;
    void clear();
    void reset();
    void reset(ID3D12CommandAllocator* pAllocator);
    void setRenderTargets(const D3dResource* pRenderTarget, uint64_t numRenderTargets, const D3dResource& depthStencil);
    void setDepthStencil(RenderTexture2D* pDepthStencil);
    void transition(D3dResource& resource, uint64_t subResourceIndex, ResourceState dstState);
    void transition(D3dResource& resource, ResourceState dstState);
    void copyResource(StaticHeap& dst,
                      const D3dResource& src);
    void drawMeshInstanced() const;
    void drawMesh(const Mesh& meshData, const DirectX::XMMATRIX& matrix, const MaterialInstance& material) const;

    ID3D12GraphicsCommandList* nativePtr() const override;
    D3D12CommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pAllocator = nullptr);
    ~D3D12CommandList() override;
    D3D12CommandList();
    
    DELETE_COPY_CONSTRUCTOR(D3D12CommandList)
    DELETE_COPY_OPERATOR(D3D12CommandList)
    DEFAULT_MOVE_CONSTRUCTOR(D3D12CommandList)
    DEFAULT_MOVE_OPERATOR(D3D12CommandList)
    
private:
    void SetShader(const HlslShader* shader);
    
    ID3D12CommandAllocator* mCommandAllocator;
    ResourceStateTracker mResourceStateTracker;
};
#endif
