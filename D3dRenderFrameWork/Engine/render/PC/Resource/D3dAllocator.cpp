#ifdef WIN32
#include "D3dAllocator.h"
#include "Engine/common/Exception.h"
#include "Engine/render/PC/Core/D3dContext.h"
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#include "Engine/render/PC/Resource/RenderTexture.h"
#include "Engine/render/PC/Resource/StaticBuffer.h"

// render layer use only
RenderTexture2D* D3dAllocator::allocDepthStencilResource(uint64_t width, uint64_t height,
                                                         TextureFormat format) const
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
    return new RenderTexture2D{ createD3dResource(D3D12_HEAP_FLAG_NONE, heapProp, dsDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue) };
}

// render layer use only
DynamicBuffer* D3dAllocator::allocDynamicBuffer(uint64_t size) const
{
    return new DynamicBuffer(createDynamicBuffer(size));
}

// render layer use only
StaticBuffer* D3dAllocator::allocStaticBuffer(uint64_t size) const
{
    return new StaticBuffer(createStaticBuffer(size));
}

// render layer use only
uint64_t D3dAllocator::allocRenderTexture2D(uint64_t width,
                                            uint64_t height, uint32_t arraySize, TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality,
                                            bool isDynamic, bool allowSimultaneous) const
{
    return reinterpret_cast<uint64_t>(new RenderTexture2D{ createD3dResource( D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(isDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format),
            width, static_cast<UINT>(height), arraySize, numMips,
            sampleCount, sampleQuality,
            allowSimultaneous ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE))
        });
}

D3dAllocator::~D3dAllocator() = default;

D3dResource D3dAllocator::createD3dResource(D3D12_HEAP_FLAGS heapFlags, const D3D12_HEAP_PROPERTIES& heapProp,
                                                    const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* pClearValue) const
{
    ID3D12Device* pDevice = mGraphicContext->deviceHandle();
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

StaticBuffer D3dAllocator::createStaticBuffer(uint64_t size) const
{
    return {createD3dResource(
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            CD3DX12_RESOURCE_DESC::Buffer(size),
            D3D12_RESOURCE_STATE_COMMON)};
}

DynamicBuffer D3dAllocator::createDynamicBuffer(uint64_t size) const
{
    return {createD3dResource(
            D3D12_HEAP_FLAG_NONE,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            CD3DX12_RESOURCE_DESC::Buffer(size),
            D3D12_RESOURCE_STATE_COMMON)};
}

D3dAllocator::D3dAllocator() = default;

D3dAllocator::D3dAllocator(D3dContext* pContext) : mGraphicContext(pContext) { } 
#endif