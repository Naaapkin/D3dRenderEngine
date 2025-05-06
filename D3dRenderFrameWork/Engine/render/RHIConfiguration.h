#pragma once
#ifdef WIN32
#include "PC/Native/D3D12RootSignatureManager.h"

struct RHIConfiguration
{
    static RHIConfiguration Default()
    {
        return {16, 16, {4, 2}};
    }
    uint16_t mMaxNumRenderTarget;
    uint16_t mMaxNumDepthStencil;
    RootSignatureLayout mRootSignatureLayout;
};
#endif