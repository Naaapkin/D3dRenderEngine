#pragma once
#include "Engine/pch.h"
#include "Engine/common/Format.h"

enum class TextureType : uint8_t
{
    TEXTURE_2D = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
    TEXTURE_3D = D3D12_RESOURCE_DIMENSION_TEXTURE3D,
};

class Texture
{
public:
    TextureType type() const;
    Format format() const;
    uint64_t width() const;
    uint64_t height() const;
    uint32_t depth() const;
    uint8_t mipLevels() const;
    uint8_t sampleCount() const;
    uint8_t sampleQuality() const;
    Texture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
            Format format, uint8_t numMips = 1,
            uint8_t sampleCount = 1, uint8_t sampleQuality = 0);
    Texture(const Texture& o) noexcept;

    virtual const byte* dataPtr() const = 0;
    virtual const byte* subDatePtr(uint8_t mip) const = 0;
    virtual ~Texture();

    DEFAULT_MOVE_CONSTRUCTOR(Texture)
    DEFAULT_MOVE_OPERATOR(Texture)
    Texture& operator=(const Texture& o) noexcept;
    
protected:
    Texture(TextureType type, const D3D12_RESOURCE_DESC& desc);
    Texture();

private:

    TextureType mType;
    Format mFormat;
    uint8_t mNumMips;
    uint8_t mSampleCount;
    uint8_t mSampleQuality;
    uint32_t mDepth;
    uint64_t mWidth;
    uint64_t mHeight;
};