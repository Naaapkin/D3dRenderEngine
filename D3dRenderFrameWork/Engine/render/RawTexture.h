#pragma once
#ifdef WIN32
#include <pch.h>
#include <Engine/render/Texture.h>

class RawTexture : public Texture
{
public:
    void SetData(const byte* data) const;
    void SetSubData(uint8_t mip, const byte* subData) const;

    byte* DataPtr() const override;
    byte* SubDatePtr(uint8_t mip, uint32_t x, uint32_t y, uint32_t z) const override;
    
    RawTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
            TextureFormat format, const byte* data = nullptr,
            uint8_t numMips = 1, uint8_t sampleCount = 1, uint8_t sampleQuality = 0);
    RawTexture(const RawTexture& o) noexcept;
    ~RawTexture() override;

    DEFAULT_MOVE_CONSTRUCTOR(RawTexture)
    DEFAULT_MOVE_OPERATOR(RawTexture)
    RawTexture& operator=(const RawTexture& o) noexcept;
    
private:
    uint64_t GetMip(uint8_t mip) const;
    uint64_t GetMipSize(uint8_t mip) const;
    
    byte* mData;
};
#endif