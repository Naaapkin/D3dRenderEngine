#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/PC/Core/D3dCommandList.h"

struct RenderData;
struct FrameResource;
class StaticBuffer;
class DynamicBuffer;
enum class TextureType : uint8_t;
enum class TextureFormat : uint8_t;
class RenderTexture2D;

class D3dAllocator
{
public:
    // ResourceHandle allocResource TODO:
    RenderTexture2D* allocDepthStencilResource(uint64_t width, uint64_t height, TextureFormat format) const;
    DynamicBuffer* allocDynamicBuffer(uint64_t size) const;
    StaticBuffer* allocStaticBuffer(uint64_t size) const;
    uint64_t allocRenderTexture2D(uint64_t width, uint64_t height, uint32_t arraySize, TextureFormat format, uint8_t numMips = 1, uint8_t sampleCount = 1, uint8_t sampleQuality = 0, bool isDynamic = false, bool allowSimultaneous = false) const;
    D3dAllocator();
    D3dAllocator(D3dContext* pContext);
    ~D3dAllocator();

    DELETE_COPY_OPERATOR(D3dAllocator)
    DELETE_COPY_CONSTRUCTOR(D3dAllocator)
    DEFAULT_MOVE_OPERATOR(D3dAllocator)
    DEFAULT_MOVE_CONSTRUCTOR(D3dAllocator)
    
private:
    D3dResource createD3dResource(D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES& heapProp, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* pClearValue = nullptr) const;
    StaticBuffer createStaticBuffer(uint64_t size) const;
    DynamicBuffer createDynamicBuffer(uint64_t size) const;
    
    D3dContext* mGraphicContext;
};
#endif