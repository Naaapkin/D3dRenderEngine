#pragma once
#include <Engine/pch.h>

#ifdef WIN32
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

struct ShaderParameter
{
    LPCSTR mSemantic;
    uint32_t mSemanticIndex;
    DXGI_FORMAT mFormat;
    uint32_t mInputSlot;
};

template<typename T>
struct alignas(256) Constant
{
    T value;

    Constant(T value) : value(value) { }
    operator T() const { return value; }
    T* operator->() { return value.operator->(); }
};

struct LightConstants
{
    // light info
    DirectX::XMVECTOR mLightDirection;
    DirectX::XMVECTOR mLightIntensity;
    DirectX::XMVECTOR mLightColor;
    DirectX::XMVECTOR mAmbientColor;
};
#endif
