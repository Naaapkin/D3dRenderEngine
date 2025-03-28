#pragma once
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/D3dObject.h"
#include "Engine/render/PC/Core/RenderContext.h"
#include "Engine/render/PC/Core/ResourceStateTracker.h"

class Material;
struct Mesh;
class D3dAllocator;
class StaticBuffer;
enum class ResourceState : uint32_t;
class D3dResource;

class Fence : public D3dObject
{
public:
    void wait(uint64_t fencePoint) const;
    ID3D12Fence* nativePtr() const override;
    // didnt guarantee gpu has reached the fence point, maybe we should wait for it
    Fence();
    Fence(ID3D12Fence* fence);
    ~Fence() override = default;

    DELETE_COPY_CONSTRUCTOR(Fence)
    DELETE_COPY_OPERATOR(Fence)
    DEFAULT_MOVE_CONSTRUCTOR(Fence)
    DEFAULT_MOVE_OPERATOR(Fence)
};

class D3dCommandList final : D3dObject
{
    friend void RenderContext::executeCommandLists(const std::vector<D3dCommandList*>& commandLists) const;
    friend void RenderContext::executeCommandList(D3dCommandList* commandList) const;
    
public:
    void close() const;
    void clear();
    void reset();
    void reset(ID3D12CommandAllocator* pAllocator);
    void setRenderTargets(const RenderTexture2D* pRenderTarget, uint64_t numRenderTargets, const RenderTexture2D& depthStencil);
    void setDepthStencil(RenderTexture2D* pDepthStencil);
    void transition(D3dResource& resource, uint64_t subResourceIndex, ResourceState dstState);
    void transition(D3dResource& resource, ResourceState dstState);
    void copyResource(StaticBuffer& dst,
                      const D3dResource& src);
    void drawMeshInstanced() const;
    void drawMesh(const Mesh& meshData, const DirectX::XMMATRIX& matrix, const Material& material) const;

    ID3D12GraphicsCommandList* nativePtr() const override;
    D3dCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12CommandAllocator* pAllocator = nullptr);
    ~D3dCommandList() override;
    D3dCommandList();
    
    DELETE_COPY_CONSTRUCTOR(D3dCommandList)
    DELETE_COPY_OPERATOR(D3dCommandList)
    DEFAULT_MOVE_CONSTRUCTOR(D3dCommandList)
    DEFAULT_MOVE_OPERATOR(D3dCommandList)
    
private:
    void SetShader(const Shader* shader);
    
    ID3D12CommandAllocator* mCommandAllocator;
    ResourceStateTracker mResourceStateTracker;
};
#endif
