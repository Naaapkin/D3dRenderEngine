#ifdef WIN32
#include "D3dAllocator.h"
#include "Engine/common/Exception.h"
#include "Engine/render/PC/Graphic.h"
#include "Engine/render/PC/Core/D3dContext.h"
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#include "Engine/render/PC/Resource/RenderTexture.h"
#include "Engine/render/PC/Resource/StaticBuffer.h"

// render layer use only
StaticHeap D3D12RHIFactory::allocDepthStencilResource(uint64_t width, uint64_t height,
                                                   Format format)
{
    D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(static_cast<DXGI_FORMAT>(format), 1.0f, 0);
    const D3D12_RESOURCE_DESC dsDesc = {
        D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        0, width, static_cast<UINT>(height), 1, 1,
        static_cast<DXGI_FORMAT>(format),
        1, 0,
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    };
    const D3D12_HEAP_PROPERTIES heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    return { createD3dResource(D3D12_HEAP_FLAG_NONE, heapProp, dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue) };
}

DynamicHeap D3D12RHIFactory::allocDynamicTexture2D(uint64_t width, uint64_t height, Format format,
                                                bool enableMipmap, bool enableMsaa, bool allowParallelAccess)
{
    return createD3dResource(
        D3D12_HEAP_FLAG_NONE,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format),
            width, height, 1,
            enableMipmap,
            enableMsaa ? GraphicSetting::gSampleCount : 1,
            enableMsaa ? GraphicSetting::gSampleQuality : 0,
            allowParallelAccess ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE),
        D3D12_RESOURCE_STATE_COMMON);
}

StaticHeap D3D12RHIFactory::allocStaticTexture2D(uint64_t width, uint64_t height, Format format,
                                              bool enableMipmap, bool enableMsaa, bool allowParallelAccess)
{
    return createD3dResource(
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format),
                width, height, 1,
                enableMipmap,
                enableMsaa ? GraphicSetting::gSampleCount : 1,
                enableMsaa ? GraphicSetting::gSampleQuality : 0,
                allowParallelAccess ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE),
            D3D12_RESOURCE_STATE_COMMON);
}

// render layer use only
DynamicHeap D3D12RHIFactory::allocDynamicBuffer(uint64_t size)
{
    return createDynamicBuffer(size);
}

// render layer use only
StaticHeap D3D12RHIFactory::allocStaticBuffer(uint64_t size)
{
    return createStaticBuffer(size);
}

D3dResource D3D12RHIFactory::createD3dResource(D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES& heapProp,
                                                    const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* pClearValue)
{
    ID3D12Device* pDevice = D3dContext::instance().deviceHandle();
    uint64_t subResourceCount = static_cast<uint64_t>(desc.MipLevels) * desc.DepthOrArraySize * D3D12GetFormatPlaneCount(pDevice, desc.Format);
    ID3D12Resource* pBuffer = nullptr;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProp,
        heapFlags,
        &desc,
        initialState,
        pClearValue,
        IID_PPV_ARGS(&pBuffer)
    ));
    return { pBuffer, subResourceCount, static_cast<ResourceState>(initialState) };
}

StaticHeap D3D12RHIFactory::createStaticBuffer(uint64_t size)
{
    return {createD3dResource(
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Buffer(size),
            D3D12_RESOURCE_STATE_COMMON)};
}

DynamicHeap D3D12RHIFactory::createDynamicBuffer(uint64_t size)
{
    return {createD3dResource(
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            CD3DX12_RESOURCE_DESC::Buffer(size),
            D3D12_RESOURCE_STATE_GENERIC_READ)};
}
#endif