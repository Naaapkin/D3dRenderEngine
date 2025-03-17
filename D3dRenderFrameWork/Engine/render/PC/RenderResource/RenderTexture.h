#pragma once
#include <Engine/render/PC/RenderResource/D3dResource.h>
#include <Engine/render/Texture.h>

#ifdef WIN32
class RenderTexture : public Texture
{
    friend RenderTexture CreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                                             TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality, bool isDynamic, bool
                                             allowSimultaneous);
    
public:
    bool AllowSimultaneous() const;
    bool IsDynamic() const;

    byte* DataPtr() const override;
    byte* SubDatePtr(uint8_t mip, uint32_t x, uint32_t y, uint32_t z) const override;
    ~RenderTexture() override;

    DEFAULT_MOVE_CONSTRUCTOR(RenderTexture)
    DEFAULT_MOVE_OPERATOR(RenderTexture)
    DELETE_COPY_OPERATOR(RenderTexture)
    DELETE_COPY_CONSTRUCTOR(RenderTexture)
    
private:
    RenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                  TextureFormat format, uint8_t numMips = 1,
                  uint8_t sampleCount = 1, uint8_t sampleQuality = 0,
                  bool isDynamic = false, bool allowSimultaneous = false);

    D3dResource mResource;
};

RenderTexture CreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                                  TextureFormat format, uint8_t numMips = 1,
                                  uint8_t sampleCount = 1, uint8_t sampleQuality = 0, bool isDynamic = false, bool allowSimultaneous = false);
#endif
