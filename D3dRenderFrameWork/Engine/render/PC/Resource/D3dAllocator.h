#pragma once
#include "D3D12Resources.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/PC/Core/D3dCommandList.h"

class D3dContext;
struct ResourceManager;
class StaticHeap;
class DynamicHeap;
enum class TextureType : uint8_t;
enum class Format : uint8_t;
class RenderTexture2D;

class D3D12RHIFactory
{
public:
    // ResourceHandle allocResource TODO:
    static StaticHeap allocDepthStencilResource(uint64_t width, uint64_t height, Format format);
    static DynamicHeap allocDynamicTexture2D(uint64_t width, uint64_t height, Format format, bool enableMipmap = true,
                                      bool enableMsaa = false, bool allowParallelAccess = false);
    static StaticHeap allocStaticTexture2D(uint64_t width, uint64_t height, Format format, bool enableMipmap = true,
                                    bool enableMsaa = false, bool allowParallelAccess = false);
    static DynamicHeap allocDynamicBuffer(uint64_t size);
    static StaticHeap allocStaticBuffer(uint64_t size);
    
private:
    static D3dResource createD3dResource(D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES& heapProp, const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON, const D3D12_CLEAR_VALUE* pClearValue = nullptr);
    static StaticHeap createStaticBuffer(uint64_t size);
    static DynamicHeap createDynamicBuffer(uint64_t size);
};

class D3D12UploadAllocator
{
public:
    D3D12Buffer* AllocBuffer(uint64_t size);
    D3D12Texture*
private:
    std::vector<D3D12Resource*> mResources;
    std::stack<uint64_t> mAvailableResourceAddresses;
    std::vector<uint64_t> mDeferredDelete;
};
#endif