#ifdef WIN32
#include <Engine/render/PC/D3dGraphicContext.h>
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/RenderResource/RenderTexture.h>
#include <Engine/render/PC/RenderResource/UploadHeap.h>
#include "Engine/common/PC/WException.h"

bool RenderTexture::AllowSimultaneous() const
{
    return mResource.NativePtr()->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
}

bool RenderTexture::IsDynamic() const
{
    D3D12_HEAP_PROPERTIES props;
    mResource.NativePtr()->GetHeapProperties(&props, nullptr);
    return props.Type == D3D12_HEAP_TYPE_DEFAULT;
}

byte* RenderTexture::DataPtr() const
{
#ifdef DEBUG || _DEBUG
    ASSERT(dynamic_cast<const UploadHeap*>(&mResource), TEXT(""))
#endif
    auto& uploadHeap = dynamic_cast<const UploadHeap&>(mResource);
    return uploadHeap.MappedPointer();
}

// avoid de-reference this pointer because it'll cause cache flush
byte* RenderTexture::SubDatePtr(uint8_t mip, uint32_t x, uint32_t y, uint32_t z) const
{
#ifdef DEBUG || _DEBUG
    ASSERT(dynamic_cast<const UploadHeap*>(&mResource), TEXT(""))
#endif
    auto& uploadHeap = dynamic_cast<const UploadHeap&>(mResource);
    D3D12_RESOURCE_DESC desc = mResource.NativePtr()->GetDesc();
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
    return uploadHeap.MappedPointer() + footprint.Offset;
}

RenderTexture::~RenderTexture() = default;

RenderTexture::RenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth, TextureFormat format,
                             uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality,
                             bool isDynamic, bool allowSimultaneous) :
    Texture(type, width, height, depth, format, numMips),
    mResource(::CreateD3dResource(::gGraphicContext()->DeviceHandle(), 
        D3D12_HEAP_FLAG_NONE,
        CD3DX12_HEAP_PROPERTIES(isDynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT),
        CD3DX12_RESOURCE_DESC::Tex2D(static_cast<DXGI_FORMAT>(format),
           width, height, depth, numMips,
           sampleCount, sampleQuality,
           allowSimultaneous ? D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS : D3D12_RESOURCE_FLAG_NONE),
        isDynamic ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON)) { }

RenderTexture CreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
    TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality, bool isDynamic,
    bool allowSimultaneous)
{
    return { type, width, height, depth, format, numMips, sampleCount, sampleQuality, isDynamic, allowSimultaneous };
}
#endif

