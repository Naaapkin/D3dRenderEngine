#pragma once
#include <pch.h>

enum class TextureFormat : uint8_t
{
    R8_UNORM = DXGI_FORMAT_R8_UNORM,
    R8G8_UNORM = DXGI_FORMAT_R8G8_UNORM,
    R8G8B8A8_UNORM = DXGI_FORMAT_R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    R8_SNORM = DXGI_FORMAT_R8_SNORM,
    R8G8_SNORM = DXGI_FORMAT_R8G8_SNORM,
    R8G8B8A8_SNORM = DXGI_FORMAT_R8G8B8A8_SNORM,
    R8_UINT = DXGI_FORMAT_R8_UINT,
    R8G8_UINT = DXGI_FORMAT_R8G8_UINT,
    R8G8B8A8_UINT = DXGI_FORMAT_R8G8B8A8_UINT,
    R8_SINT = DXGI_FORMAT_R8_SINT,
    R8G8_SINT = DXGI_FORMAT_R8G8_SINT,
    R8G8B8A8_SINT = DXGI_FORMAT_R8G8B8A8_SINT,
    R16_UNORM = DXGI_FORMAT_R16_UNORM,
    R16G16_UNORM = DXGI_FORMAT_R16G16_UNORM,
    R16G16B16A16_UNORM = DXGI_FORMAT_R16G16B16A16_UNORM,
    R16_SNORM = DXGI_FORMAT_R16_SNORM,
    R16G16_SNORM = DXGI_FORMAT_R16G16_SNORM,
    R16G16B16A16_SNORM = DXGI_FORMAT_R16G16B16A16_SNORM,
    R16_UINT = DXGI_FORMAT_R16_UINT,
    R16G16_UINT = DXGI_FORMAT_R16G16_UINT,
    R16G16B16A16_UINT = DXGI_FORMAT_R16G16B16A16_UINT,
    R16_SINT = DXGI_FORMAT_R16_SINT,
    R16G16_SINT = DXGI_FORMAT_R16G16_SINT,
    R16G16B16A16_SINT = DXGI_FORMAT_R16G16B16A16_SINT,
    R32_TYPELESS = DXGI_FORMAT_R32_TYPELESS,
    R32G32_TYPELESS = DXGI_FORMAT_R32G32_TYPELESS,
    R32G32B32A32_TYPELESS = DXGI_FORMAT_R32G32B32A32_TYPELESS,
    R32_FLOAT = DXGI_FORMAT_R32_FLOAT,
    R32G32_FLOAT = DXGI_FORMAT_R32G32_FLOAT,
    R32G32B32A32_FLOAT = DXGI_FORMAT_R32G32B32A32_FLOAT,
};

enum class TextureType : uint8_t
{
    TEXTURE_2D,
    TEXTURE_3D,
    TEXTURE_CUBE,
    TEXTURE_ARRAY,
};

class Texture
{
public:
    TextureType Type() const;
    TextureFormat Format() const;
    uint32_t Width() const;
    uint32_t Height() const;
    uint32_t Depth() const;
    uint8_t MipLevels() const;
    uint8_t SampleCount() const;
    uint8_t SampleQuality() const;
    Texture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
            TextureFormat format, uint8_t numMips = 1,
            uint8_t sampleCount = 1, uint8_t sampleQuality = 0);
    Texture(const Texture& o) noexcept;

    virtual byte* DataPtr() const = 0;
    virtual byte* SubDatePtr(uint8_t mip, uint32_t x, uint32_t y, uint32_t z) const = 0;
    virtual ~Texture();

    DEFAULT_MOVE_CONSTRUCTOR(Texture)
    DEFAULT_MOVE_OPERATOR(Texture)
    Texture& operator=(const Texture& o) noexcept;

private:
    Texture();

    TextureType mType;
    TextureFormat mFormat;
    uint8_t mNumMips;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;
    uint8_t mSampleCount;
    uint8_t mSampleQuality;
};
