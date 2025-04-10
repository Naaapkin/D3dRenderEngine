#pragma once
#include "Shader.h"
#include "Engine/pch.h"

struct ResourceHandle;
struct RenderList;

class Renderer
{
public:
    static Renderer* sRenderer();
    virtual void initialize(RendererSetting setting) = 0;
    virtual void registerShaders(std::vector<Shader*> shaders) = 0;
    virtual ResourceHandle allocateDynamicBuffer(uint64_t size) = 0;
    virtual ResourceHandle RHICreateVertexBuffer(uint32_t numVertices, uint16_t vertexSize) = 0;
    virtual ResourceHandle RHICreateIndexBuffer(uint64_t size) = 0;
    virtual void updateDynamicResource(const ResourceHandle& resourceHandle, const void* data) const = 0;
    virtual void updateStaticResource(const ResourceHandle& resourceHandle, const void* data) const = 0;
    virtual void releaseResource(const ResourceHandle& resourceHandle) const = 0;
    virtual void updatePassConstants(uint8_t registerIndex, void* pData, uint64_t size) = 0;
    virtual void appendRenderLists(std::vector<RenderList>&& renderLists) = 0;
    virtual void render() = 0;
    virtual void release() = 0;
    virtual ~Renderer();
    
    DELETE_COPY_CONSTRUCTOR(Renderer)
    DELETE_COPY_OPERATOR(Renderer)
    DEFAULT_MOVE_CONSTRUCTOR(Renderer)
    DEFAULT_MOVE_OPERATOR(Renderer)

protected:
    Renderer() = default;
};
