#pragma once
#include "Engine/pch.h"
#include "Engine/common/helper.h"
class RHIShader;

enum class Format : uint8_t
{
    R8_UNORM,
    R8G8_UNORM,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8_SNORM,
    R8G8_SNORM,
    R8G8B8A8_SNORM,
    R8_UINT,
    R8G8_UINT,
    R8G8B8A8_UINT,
    R8_SINT,
    R8G8_SINT,
    R8G8B8A8_SINT,
    R16_UNORM,
    R16G16_UNORM,
    R16G16B16A16_UNORM,
    R16_SNORM,
    R16G16_SNORM,
    R16G16B16A16_SNORM,
    R16_UINT,
    R16G16_UINT,
    R16G16B16A16_UINT,
    R16_SINT,
    R16G16_SINT,
    R16G16B16A16_SINT,
    R32_TYPELESS,
    R32G32_TYPELESS,
    R32G32B32A32_TYPELESS,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32A32_FLOAT,
    D24_UNORM_S8_UINT,
    
    UNKNOWN,
};

enum class ResourceUsage : uint8_t
{
    DYNAMIC,
    STATIC,
    SHADER_RESOURCE,
    UNORDERED_ACCESS,
    NONE,
};

enum class TextureDimension : uint8_t
{
    TEXTURE1D,
    TEXTURE2D,
    TEXTURE_ARRAY,
    TEXTURE3D,
};

enum class FrameBufferType : uint8_t
{
    COLOR,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,
};

enum class CommandQueueType : uint8_t
{
    GRAPHIC,
    COPY,
    COMPUTE
};

#undef DOMAIN

enum class VertexSegment : uint8_t
{
    POSITION,
    NORMAL,
    TANGENT,
    BITANGENT,
    COLOR,
    TEXCOORD
};

enum class ShaderPropType : uint8_t
{
    CBUFFER = D3D_SIT_CBUFFER,
    TEXTURE = D3D_SIT_TEXTURE,
    SAMPLER = D3D_SIT_SAMPLER,
};

enum class ShaderType : uint8_t
{
    NONE = 0,
    VERTEX = 0b00001,
    HULL = 0b00010,
    DOMAIN = 0b00100,
    GEOMETRY = 0b01000,
    PIXEL = 0b10000
};

struct RHIBufferDesc final
{
    RHIBufferDesc() : mSize(0), mResourceType(ResourceUsage::NONE) { }
    RHIBufferDesc(uint64_t size, ResourceUsage resourceType) : mSize(size), mResourceType(resourceType) { }
    
    uint64_t mSize;
    ResourceUsage mResourceType;
};

struct RHITextureDesc final
{
    RHITextureDesc() : mFormat(Format::UNKNOWN), mDimension(TextureDimension::TEXTURE2D), mWidth(0), mHeight(0), mDepth(0), mMipLevels(0),
                       mSampleCount(1), mSampleQuality(0)
    {
    }
    RHITextureDesc(Format format, TextureDimension dimension, uint32_t width, uint32_t height, uint32_t depth, uint8_t mipLevels,
                   uint8_t sampleCount, uint8_t sampleQuality) :
        mFormat(format), mDimension(dimension), mWidth(width), mHeight(height), mDepth(depth), mMipLevels(mipLevels),
        mSampleCount(sampleCount), mSampleQuality(sampleQuality)
    {
    };

    Format mFormat;
    TextureDimension mDimension;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;
    uint8_t mMipLevels;
    uint8_t mSampleCount;
    uint8_t mSampleQuality;
};

struct Viewport
{
    Viewport(float width, float height, float minDepth, float maxDepth)
        : mTop(0.0f), mLeft(0.0f), mWidth(width), mHeight(height),
          mMinDepth(minDepth), mMaxDepth(maxDepth) {}
    float mTop;
    float mLeft;
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

struct TextureCopyLocation
{
    uint32_t mMipmap;
    uint32_t mArrayIndex;
    uint32_t mPosX;
    uint32_t mPosY;
    uint32_t mPosZ;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;

    static TextureCopyLocation BufferLocation(uint64_t src, uint64_t size)
    {
        TextureCopyLocation copy{};
        copy.mPosX = src;
        copy.mWidth = size;
        return copy;
    }

    static TextureCopyLocation Texture2DLocation(uint32_t mipmap, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mPosX = posX;
        copy.mPosY = posY;
        copy.mWidth = width;
        copy.mHeight = height;
        return copy;
    }

    static TextureCopyLocation TextureArrayLocation(uint32_t mipmap, uint32_t arrayIndex, uint32_t posX, uint32_t posY, uint32_t width, uint32_t height)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mArrayIndex = arrayIndex;
        copy.mPosX = posX;
        copy.mPosY = posY;
        copy.mWidth = width;
        copy.mHeight = height;
        return copy;
    }

    static TextureCopyLocation Texture3DLocation(uint32_t mipmap, uint32_t posX, uint32_t posY, uint32_t posZ, uint32_t width, uint32_t height, uint32_t depth)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mPosX = posX;
        copy.mPosY = posY;
        copy.mPosZ = posZ;
        copy.mWidth = width;
        copy.mHeight = height;
        copy.mDepth = depth;
        return copy;
    }
};

enum class BlendOperation : uint8_t
{
    ADD	= 1,
    SUBTRACT	= 2,
    REV_SUBTRACT	= 3,
    MIN	= 4,
    MAX	= 5
};

enum class ColorMask : uint8_t
{
    RED	= 1,
    GREEN = 2,
    BLUE = 4,
    ALPHA = 8,
    ALL	= RED | GREEN  | BLUE  | ALPHA
};

enum class BlendMode : uint8_t
{
    ZERO = 1,
    ONE	= 2,
    SRC_COLOR = 3,
    INV_SRC_COLOR = 4,
    SRC_ALPHA = 5,
    INV_SRC_ALPHA = 6,
    DEST_ALPHA = 7,
    INV_DEST_ALPHA = 8,
    DEST_COLOR = 9,
    INV_DEST_COLOR = 10,
    SRC_ALPHA_SAT = 11,
    BLEND_FACTOR = 14,
    INV_BLEND_FACTOR = 15,
    SRC1_COLOR = 16,
    INV_SRC1_COLOR = 17,
    SRC1_ALPHA = 18,
    INV_SRC1_ALPHA	= 19,
    ALPHA_FACTOR = 20,
    INV_ALPHA_FACTOR = 21
};

enum class LogicOperation : uint8_t
{
    CLEAR	= 0,
    SET	= ( CLEAR + 1 ) ,
    COPY	= ( SET + 1 ) ,
    COPY_INVERTED	= ( COPY + 1 ) ,
    NOOP	= ( COPY_INVERTED + 1 ) ,
    INVERT	= ( NOOP + 1 ) ,
    AND	= ( INVERT + 1 ) ,
    NAND	= ( AND + 1 ) ,
    OR	= ( NAND + 1 ) ,
    NOR	= ( OR + 1 ) ,
    XOR	= ( NOR + 1 ) ,
    EQUIV	= ( XOR + 1 ) ,
    AND_REVERSE	= ( EQUIV + 1 ) ,
    AND_INVERTED	= ( AND_REVERSE + 1 ) ,
    OR_REVERSE	= ( AND_INVERTED + 1 ) ,
    OR_INVERTED	= ( OR_REVERSE + 1 ) 
};

enum class DrawMode : uint8_t
{
    SOLID,
    WIREFRAME,
    POINT
};

enum class CullMode : uint8_t
{
    FRONT,
    BACK,
    NONE
};

enum class CompareFunction: uint8_t
{
    NEVER = 1,
    LESS = 2,
    EQUAL = 3,
    LESS_EQUAL = 4,
    GREATER	= 5,
    NOT_EQUAL = 6,
    GREATER_EQUAL = 7,
    ALWAYS = 8
};

enum class StencilOperation: uint8_t
{
    KEEP = 1,
    ZERO = 2,
    REPLACE	= 3,
    INCR_SAT = 4,
    DECR_SAT = 5,
    INVERT = 6,
    INCR = 7,
    DECR = 8
};

enum class DepthOperation: uint8_t
{
    READ_ONLY = 0,
    WRITE = 1,
};

enum class DepthBiasSet : uint8_t
{
    NONE,
    NORMAL,
    SHADOW_MAP,
    TERRAIN
};

enum class BlendType : uint8_t
{
    LOGIC = 0,
    COLOR = 1,
};

enum class PrimitiveType : uint8_t
{
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
};

struct RasterizerDesc
{
    DrawMode mDrawMode;
    CullMode mCullMode;
    DepthBiasSet mDepthBias;
    uint8_t mEnableMsaa;
    uint8_t mEnableConservativeMode;

    static constexpr RasterizerDesc Default()
    {
        static RasterizerDesc desc = {DrawMode::SOLID, CullMode::BACK, DepthBiasSet::NONE, false, false};
        return desc;
    }

    bool operator==(const RasterizerDesc& other) const
    {
        return mDrawMode == other.mDrawMode && mCullMode == other.mCullMode && mDepthBias == other.mDepthBias && mEnableMsaa == other.mEnableMsaa && mEnableConservativeMode == other.mEnableConservativeMode;
    }
};

struct DepthTestDesc
{
    uint8_t mEnableDepthTest;
    CompareFunction mCompareFunction;
    DepthOperation mDepthOperation;
    uint8_t mDepthWriteMask;

    static DepthTestDesc Default()
    {
        static DepthTestDesc defaultDepth = {true, CompareFunction::LESS, DepthOperation::WRITE, 0};
        return defaultDepth;
    }

    bool operator==(const DepthTestDesc& other) const
    {
        return mEnableDepthTest == other.mEnableDepthTest && mDepthWriteMask == other.mDepthWriteMask && mCompareFunction == other.mCompareFunction && mDepthWriteMask == other.mDepthWriteMask;
    }
};

struct StencilTestDesc
{
    uint8_t mEnableStencilTest;
    uint8_t mStencilReadMask;
    uint8_t mStencilWriteMask;
    StencilOperation mFrontStencilFailOp;
    StencilOperation mFrontStencilDepthFailOp;
    StencilOperation mFrontStencilPassOp;
    CompareFunction mFrontStencilFunc;
    StencilOperation mBackStencilFailOp;
    StencilOperation mBackStencilDepthFailOp;
    StencilOperation mBackStencilPassOp;
    CompareFunction mBackStencilFunc;
    
    static StencilTestDesc Default()
    {
        static StencilTestDesc defaultStencil = {true, 0xf, 0xf, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS };
        return defaultStencil;
    }
};

struct BlendDesc
{
    BlendType mBlendType;
    BlendMode mSrcBlend;
    BlendMode mDestBlend;
    BlendOperation mBlendOp;
    BlendMode mSrcBlendAlpha;
    BlendMode mDestBlendAlpha;
    BlendOperation mBlendOpAlpha;
    LogicOperation mLogicOp;
    ColorMask mRenderTargetWriteMask;

    static BlendDesc Logic(LogicOperation logicOp, ColorMask colorMask)
    {
        static BlendDesc desc = {BlendType::LOGIC, BlendMode::ZERO, BlendMode::ONE, BlendOperation::ADD, BlendMode::ZERO, BlendMode::ONE, BlendOperation::ADD, logicOp, colorMask};
        return desc;
    }

    static BlendDesc Color(BlendMode srcBlend = BlendMode::ONE, BlendMode destBlend = BlendMode::ZERO, BlendOperation blendOp = BlendOperation::ADD,
                                BlendMode srcBlendAlpha = BlendMode::ONE, BlendMode destBlendAlpha = BlendMode::ZERO, BlendOperation blendOpAlpha = BlendOperation::ADD,
                                ColorMask renderTargetWriteMask = ColorMask::ALL)
    {
        static BlendDesc desc = { BlendType::COLOR, srcBlend, destBlend, blendOp, srcBlendAlpha, destBlendAlpha, blendOpAlpha, LogicOperation::CLEAR, renderTargetWriteMask};
        return desc;
    }
};

struct PSOInitializer
{
    void SetDepthTest(const DepthTestDesc& depthTest)
    {
        mOptions.mEnableDepthTest = depthTest.mEnableDepthTest;
        if (mOptions.mEnableDepthTest)
        {
            mDepth.mCompareFunction = depthTest.mCompareFunction;
            mDepth.mDepthOperation = depthTest.mDepthOperation;
            mDepth.mDepthWriteMask = depthTest.mDepthWriteMask;
        }
        else
        {
            mDepth.mInit = 0;
        }
    }

    void SetStencilTest(const StencilTestDesc& stencilTest)
    {
        mOptions.mEnableStencilTest = stencilTest.mEnableStencilTest;
        if (mOptions.mEnableStencilTest)
        {
            mStencil.mStencilReadMask = stencilTest.mStencilReadMask;
            mStencil.mStencilWriteMask = stencilTest.mStencilWriteMask; 
            mStencil.mBackStencilFunc = stencilTest.mBackStencilFunc;
            mStencil.mBackStencilFailOp = stencilTest.mBackStencilFailOp;
            mStencil.mBackStencilDepthFailOp = stencilTest.mBackStencilDepthFailOp;
            mStencil.mBackStencilPassOp = stencilTest.mBackStencilPassOp;
            mStencil.mFrontStencilFunc = stencilTest.mFrontStencilFunc;
            mStencil.mFrontStencilFailOp = stencilTest.mFrontStencilFailOp;
            mStencil.mFrontStencilDepthFailOp = stencilTest.mFrontStencilDepthFailOp;
            mStencil.mFrontStencilPassOp = stencilTest.mFrontStencilPassOp;
        }
        else
        {
            mStencil.mInit[0] = 0;
            mStencil.mInit[1] = 0;
        }
    }

    void SetRasterizer(const RasterizerDesc& rasterizer)
    {
        mRasterizer.mCullMode = rasterizer.mCullMode;
        mRasterizer.mDrawMode = rasterizer.mDrawMode;
        mRasterizer.mDepthBias = rasterizer.mDepthBias;
    }

    void SetBlend(const BlendDesc& blend)
    {
        mBlendType = blend.mBlendType;
        if (blend.mBlendType == BlendType::LOGIC)
        {
            mBlend.mInit = 0;
            mBlend.mLogicOp = blend.mLogicOp;
        }
        else
        {
            mBlend.mSrcBlend = blend.mSrcBlendAlpha;
            mBlend.mDestBlend = blend.mDestBlendAlpha;
            mBlend.mBlendOp = blend.mBlendOp;
            mBlend.mSrcBlendAlpha = blend.mSrcBlendAlpha;
            mBlend.mDestBlendAlpha = blend.mDestBlendAlpha;
            mBlend.mBlendOpAlpha = blend.mBlendOpAlpha;
        }
    }

    bool operator==(const PSOInitializer& other) const
    {
        bool sameBlend = mBlendType == other.mBlendType && mRenderTargetWriteMask == other.mRenderTargetWriteMask && (mBlendType == BlendType::LOGIC ? mBlend.mLogicOp == other.mBlend.mLogicOp : mBlend.mInit == other.mBlend.mInit);
        bool sameOptions = mOptions.mInit == other.mOptions.mInit;
        if (!sameBlend || !sameOptions) return false;
        bool sameDepth = mOptions.mEnableDepthTest ?  mDepth.mInit == other.mDepth.mInit : true;
        return sameDepth && (mOptions.mEnableStencilTest ?  mStencil.mInit[0] == other.mStencil.mInit[0] && mStencil.mInit[1] == other.mStencil.mInit[1] : true);
    }

    uint64_t Hash() const
    {
        uint64_t hash = static_cast<uint8_t>(mBlendType) | (static_cast<uint8_t>(mRenderTargetWriteMask) << 8) | (mOptions.mInit << 16) | (mRasterizer.mInit << 32);
        hash = MurmurHash(hash, mBlend.mInit);
        hash = MurmurHash(hash, mDepth.mInit);
        hash = MurmurHash(hash, mStencil.mInit[0]);
        hash = MurmurHash(hash, mStencil.mInit[1]);
        hash = MurmurHash(hash, reinterpret_cast<uint64_t>(mShader));
        return hash;
    }

    union
    {
        struct
        {
            bool mEnableDepthTest;
            bool mEnableStencilTest;
            bool mEnableMsaa;
            bool mEnableConservativeMode;
        };
        uint8_t mInit{};
    } mOptions;
    
    BlendType mBlendType;
    ColorMask mRenderTargetWriteMask;
    union
    {
        struct
        {
            BlendMode mSrcBlend;
            BlendMode mDestBlend;
            BlendOperation mBlendOp;
            BlendMode mSrcBlendAlpha;
            BlendMode mDestBlendAlpha;
            BlendOperation mBlendOpAlpha;
            uint8_t mPadding[2];
        };
        uint64_t mInit{};
        LogicOperation mLogicOp;
    } mBlend;

    union StencilTest
    {
        struct
        {
            uint8_t mStencilReadMask;
            uint8_t mStencilWriteMask;
            StencilOperation mFrontStencilFailOp;
            StencilOperation mFrontStencilDepthFailOp;
            StencilOperation mFrontStencilPassOp;
            CompareFunction mFrontStencilFunc;
            StencilOperation mBackStencilFailOp;
            StencilOperation mBackStencilDepthFailOp;
            StencilOperation mBackStencilPassOp;
            CompareFunction mBackStencilFunc;
            uint8_t mPadding[6];
        };
        uint64_t mInit[2]{};
    } mStencil;

    union DepthTest
    {
        struct
        {
            CompareFunction mCompareFunction;
            DepthOperation mDepthOperation;
            uint8_t mDepthWriteMask;
            uint8_t mPadding[1];
        };
        uint32_t mInit{};
    } mDepth;

    union Rasterizer
    {
        struct
        {
            DrawMode mDrawMode;
            CullMode mCullMode;
            DepthBiasSet mDepthBias;
        };
        uint32_t mInit{};
    } mRasterizer;

    RHIShader* mShader;
};

template <>
struct std::hash<PSOInitializer>
{
    size_t operator()(const PSOInitializer& key) const noexcept
    {
        return key.Hash();
    }
};

struct GraphicPipelineStateDesc
{
    RasterizerDesc mRasterizerDesc;
    Format mDepthStencilFormat;
    Format mRenderTargetFormats[8];
    PrimitiveType mPrimitiveType;
    DepthTestDesc mDepthTestDesc;
    StencilTestDesc mStencilTestDesc;
    RHIShader* mShader;
    BlendDesc mBlendDesc;

    static GraphicPipelineStateDesc Default()
    {
        static GraphicPipelineStateDesc desc = {RasterizerDesc::Default(), Format::D24_UNORM_S8_UINT,
            {Format::R32G32B32A32_FLOAT},
            PrimitiveType::TRIANGLE_LIST,
            DepthTestDesc::Default(),
            StencilTestDesc::Default(),
            nullptr,
            BlendDesc::Color(),
            };
        return desc;
    } 
};
