#ifdef WIN32
#include "Engine/render/PC/Resource/RenderTexture.h"
#include "Engine/render/PC/Resource/DynamicBuffer.h"
#include "Engine/common/Exception.h"

bool RenderTexture2D::allowSimultaneous() const
{
    return nativePtr()->GetDesc().Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
}

bool RenderTexture2D::isDynamic() const
{
    D3D12_HEAP_PROPERTIES props;
    nativePtr()->GetHeapProperties(&props, nullptr);
    return props.Type == D3D12_HEAP_TYPE_UPLOAD;
}

const byte* RenderTexture2D::dataPtr() const
{
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(isDynamic(), TEXT("cant map data to default heap\n"))
#endif
    return mMappedPointer;
}

// avoid de-referencing this pointer, it'll cause gpu cache flush
const byte* RenderTexture2D::subDatePtr(uint8_t mip) const
{
#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(isDynamic(), TEXT("Texture is static\n"))
#endif
    D3D12_RESOURCE_DESC desc = nativePtr()->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
    ID3D12Device* pDevice;
    nativePtr()->GetDevice(IID_PPV_ARGS(&pDevice));
    pDevice->GetCopyableFootprints(
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

void RenderTexture2D::release()
{
    if (nativePtr() && isDynamic())
    {
        D3D12_RANGE range = {0, 0};
        nativePtr()->Unmap(0, &range);
    }
    D3dResource::release();
}

RenderTexture2D::RenderTexture2D() : mMappedPointer(nullptr) { }

RenderTexture2D::RenderTexture2D(D3dResource&& resource) : Texture(TextureType::TEXTURE_2D, resource.nativePtr()->GetDesc()), D3dResource(std::move(resource)), mMappedPointer(nullptr)
{
    if (!isDynamic()) return;
    constexpr D3D12_RANGE range = {0, 0};
    void* mappedPointer;
    resource.nativePtr()->Map(0, &range, &mappedPointer);
    mMappedPointer = static_cast<const byte*>(mappedPointer);
}

RenderTexture2D::RenderTexture2D(RenderTexture2D&& other) noexcept
{
}

RenderTexture2D& RenderTexture2D::operator=(RenderTexture2D&& other) noexcept
{
}

RenderTexture2D::~RenderTexture2D() = default;
#endif

