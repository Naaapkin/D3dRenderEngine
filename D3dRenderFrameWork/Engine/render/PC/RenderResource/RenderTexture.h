#pragma once
#include <Engine/render/PC/RenderResource/D3dResource.h>
#include <Engine/render/Texture.h>

#ifdef WIN32
class RenderTexture final : public Texture, public D3dResource
{
    friend RenderTexture gCreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                                             TextureFormat format, uint8_t numMips, uint8_t sampleCount, uint8_t sampleQuality, bool isDynamic, bool
                                             allowSimultaneous);
    friend class D3dRenderer;
    
public:
    bool allowSimultaneous() const;
    bool isDynamic() const;

    const byte* dataPtr() const override;
    const byte* subDatePtr(uint8_t mip) const override;
    void release() override;
    ~RenderTexture() override;

    DEFAULT_MOVE_CONSTRUCTOR(RenderTexture)
    DEFAULT_MOVE_OPERATOR(RenderTexture)
    DELETE_COPY_OPERATOR(RenderTexture)
    DELETE_COPY_CONSTRUCTOR(RenderTexture)
    
private:
    RenderTexture(TextureType type, D3dResource&& resource);

    const byte* mMappedPointer;
};

RenderTexture gCreateRenderTexture(TextureType type, uint64_t width, uint64_t height, uint32_t depth,
                                  TextureFormat format, uint8_t numMips = 1,
                                  uint8_t sampleCount = 1, uint8_t sampleQuality = 0, bool isDynamic = false, bool allowSimultaneous = false);
#endif
