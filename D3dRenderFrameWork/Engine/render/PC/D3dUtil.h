#pragma once
#ifdef WIN32
#include "Engine/pch.h"

struct Viewport
{
    Viewport(float width, float height, float minDepth, float maxDepth)
        : mTopX(0.0f), mLeftY(0.0f), mWidth(width), mHeight(height),
          mMinDepth(minDepth), mMaxDepth(maxDepth) {}
    float mTopX;
    float mLeftY;
    float mWidth;
    float mHeight;
    float mMinDepth;
    float mMaxDepth;
};

struct Rect
{
    Rect(LONG left, LONG top, LONG right, LONG bottom)
        : mLeft(left), mTop(top), mRight(right), mBottom(bottom) {}
    LONG mLeft;
    LONG mTop;
    LONG mRight;
    LONG mBottom;
};

template<typename T>
struct alignas(256) Constant
{
    T value;

    Constant(T value) : value(value) { }
    operator T() const { return value; }
    T* operator->() { return value.operator->(); }
};

struct PassData
{
    // light info
    DirectX::XMVECTOR mLightDirection;
    DirectX::XMVECTOR mLightIntensity;
    DirectX::XMVECTOR mLightColor;
    DirectX::XMVECTOR mAmbientColor;
};

DXGI_FORMAT GetParaInfoFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
ID3DBlob* LoadCompiledShaderObject(const String& path);
D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPipelineStateDesc();
bool gImplicitTransit(uint32_t stateBefore, uint32_t& stateAfter, bool isBufferOrSimultaneous);
#endif
