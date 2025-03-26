#pragma once
#include "Engine/render/PC/Resource/D3dResource.h"
#include "Engine/render/Texture.h"

#ifdef WIN32
class RenderTexture2D final : public Texture, public D3dResource
{
public:
    bool allowSimultaneous() const;
    bool isDynamic() const;

    const byte* dataPtr() const override;
    const byte* subDatePtr(uint8_t mip) const override;
    void release() override;
    
    RenderTexture2D();
    RenderTexture2D(D3dResource&& resource);
    ~RenderTexture2D() override;

    DEFAULT_MOVE_CONSTRUCTOR(RenderTexture2D)
    DEFAULT_MOVE_OPERATOR(RenderTexture2D)
    DELETE_COPY_OPERATOR(RenderTexture2D)
    DELETE_COPY_CONSTRUCTOR(RenderTexture2D)
    
private:
    const byte* mMappedPointer;
};
#endif
