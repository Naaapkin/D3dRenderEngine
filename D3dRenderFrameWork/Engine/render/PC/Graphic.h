#pragma once
#include "Engine/pch.h"
#include "Engine/render/Texture.h"

struct RendererSetting
{
    Format mBackBufferFormat = Format::R8G8B8A8_UNORM;
    Format mDepthStencilFormat = Format::D24_UNORM_S8_UINT;
    uint8_t mMsaaSampleCount = 4;
    uint8_t mMsaaSampleQuality = 1;
    uint8_t mNumBackBuffers = 3;
};

namespace GraphicSetting
{
    Format gBackBufferFormat = Format::R8G8B8A8_UNORM;
    Format gDepthStencilFormat = Format::D24_UNORM_S8_UINT;
    uint8_t gSampleCount = 4;
    uint8_t gSampleQuality = 1;
    
    uint64_t gNumPassConstants = 2;
    uint64_t gNumPerObjectConstants = 2;
    uint64_t gMaxRenderItemsPerFrame = 256;
    uint64_t gNumGlobalTexture = 4;
    uint8_t gNumBackBuffers = 3;
    uint16_t gMaxNumRenderTarget = 8;
}
