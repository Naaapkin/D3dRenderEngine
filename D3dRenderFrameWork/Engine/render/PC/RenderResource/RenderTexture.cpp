#ifdef WIN32
#include "Engine/render/PC/Core/D3dGraphicContext.h"
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/RenderResource/RenderTexture.h>
#include <Engine/render/PC/RenderResource/DynamicBuffer.h>
#include "Engine/common/Exception.h"

bool RenderTexture::allowSimultaneous() const
{
    return NativePtr()->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
}

bool RenderTexture::isDynamic() const
{
    D3D12_HEAP_PROPERTIES props;
    NativePtr()->GetHeapProperties(&props, nullptr);
    return props.Type == D3D12_HEAP_TYPE_UPLOAD;
}

const byte* RenderTexture::dataPtr() const
{
#ifdef DEBUG || _DEBUG
    ASSERT(isDynamic(), TEXT(""))
#endif
    return mMappedPointer;
}

// avoid de-reference this pointer because it'll cause cache flush
const byte* RenderTexture::subDatePtr(uint8_t mip) const
{
#ifdef DEBUG || _DEBUG
    ASSERT(isDynamic(), TEXT("Texture is static\n"))
#endif
    D3D12_RESOURCE_DESC desc = NativePtr()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    gGraphicContext()->DeviceHandle()->GetCopyableFootprints(
        &desc,              // 资源描述
        mip,                // 起始 Mip Level
        1,                  // 计算的 Mip Levels 数量
        0,                  // 偏移量（一般为 0）
        &footprint,         // 输出：每个 Mip Level 的内存布局信息
        nullptr,
        nullptr,
        nullptr       
    );
    return mMappedPointer + footprint.Offset;
}

void RenderTexture::release()
{
    if (NativePtr() && isDynamic())
    {
        D3D12_RANGE range = {0, 0};
        NativePtr()->Unmap(0, &range);
    }
    D3dResource::release();
}

RenderTexture::~RenderTexture() = default;

RenderTexture::RenderTexture(TextureType type, D3dResource&& resource) :
    Texture(type, resource.NativePtr()->GetDesc()),
    D3dResource(std::move(resource))
{
    D3D12_RANGE range = {0, 0};
    void* mappedPointer;
    resource.NativePtr()->Map(0, &range, &mappedPointer);
    mMappedPointer = static_cast<const byte*>(mappedPointer);
}

RenderTexture gCreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                                  TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality, bool isDynamic,
                                  bool allowSimultaneous)
{
    return { type, ::gCreateD3dResource(::gGraphicContext()->DeviceHandle(), 
        D3D12_HEAP_FLAG_NONE,
        CD3DX12_HEAP_PROPERTIES(isDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format),
           width, height, depth, numMips,
           sampleCount, sampleQuality,
           allowSimultaneous ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE),
        isDynamic ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON) };
}
#endif

