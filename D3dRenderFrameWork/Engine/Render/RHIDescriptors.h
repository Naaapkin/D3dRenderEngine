#pragma once
#include "Engine/pch.h"

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
    R32_UINT,
    R32G32_UINT,
    R32G32B32_UINT,
    R32G32B32A32_UINT,
    R32_SINT,
    R32G32_SINT,
    R32G32B32_SINT,
    R32G32B32A32_SINT,
    R32_FLOAT,
    R32G32_FLOAT,
    R32G32B32_FLOAT,
    R32G32B32A32_FLOAT,
    D24_UNORM_S8_UINT,
    
    UNKNOWN,
};

enum class ResourceType : uint8_t
{
    DYNAMIC,
    STATIC,
    SHADER_RESOURCE,
    UNORDERED_ACCESS,
    NONE,
};

enum class ResourceDimension : uint8_t
{
    BUFFER,
    TEXTURE1D,
    TEXTURE1D_ARRAY,
    TEXTURE2D,
    TEXTURE2D_ARRAY,
    TEXTURE3D,
    TEXTURE_CUBE,
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
    CBUFFER,
    TBUFFER,
    TEXTURE,
    SAMPLER
};

enum ShaderType : uint8_t
{
    NONE = 0,
    VERTEX = 0b00001,
    HULL = 0b00010,
    DOMAIN = 0b00100,
    GEOMETRY = 0b01000,
    PIXEL = 0b10000
};

enum UpdateType : uint8_t
{
    PER_FRAME,
    PER_PASS,
    PER_DRAW
};

enum class CommandListType : uint8_t
{
    GRAPHIC = 0,
    COPY = 1,
    COMPUTE
};

struct ShaderProp
{
    uint8_t mRegister;
    ShaderPropType mType;
    ShaderType mVisibility;
    union
    {
		uint64_t mCBufferSize;
		ResourceDimension mDimension;
    } mInfo;
    std::string mName;

    bool operator==(const ShaderProp& rhs) const noexcept
    {
        return mRegister == rhs.mRegister && mType == rhs.mType;
    }
};

template <>
struct std::hash<ShaderProp>
{
    size_t operator()(const ShaderProp& key) const noexcept
    {
        return std::hash<UINT16>()(static_cast<uint16_t>(key.mType) << 8 | key.mRegister);
    }
};

struct ShaderInput
{
    Format mFormat;
    std::string mSemanticName;
    uint32_t mSemanticIndex;
    uint32_t mInputSlot;
};

struct RHISwapChainDesc
{
    uint32_t mTargetFramesPerSec;
    uint32_t mWidth;
    uint32_t mHeight;
    uint8_t mMSAA;
    uint8_t mNumBackBuffers;
    Format mFormat;
    bool mIsFullScreen;
};

struct RHIShaderMeta
{
    std::string mName;
    std::vector<ShaderInput> mInputElements;
    std::unordered_set<ShaderProp> mProperties;
};

struct RegisterLayout
{
    uint8_t mNumConstantsPerMaterial;
    uint8_t mNumGlobalTextures;
    uint8_t mMaxMaterialTextures;
};

struct RHIBufferDesc final
{
    RHIBufferDesc() : mSize(0), mStride(sizeof(float[4])), mResourceType(ResourceType::NONE) { }
    RHIBufferDesc(uint64_t size, uint32_t stride, ResourceType resourceType) : mSize(size), mStride(stride), mResourceType(resourceType) { }
    
    uint64_t mSize;
    uint32_t mStride;
    ResourceType mResourceType;
};

struct RHITextureDesc final
{
    RHITextureDesc() : mWidth(0), mHeight(0), mDepth(0), mFormat(Format::UNKNOWN), mDimension(ResourceDimension::TEXTURE2D), mMipLevels(0),
                       mSampleCount(1), mSampleQuality(0) { }
    RHITextureDesc(Format format, ResourceDimension dimension, uint32_t width, uint32_t height, uint32_t depth, uint8_t mipLevels,
                   uint8_t sampleCount, uint8_t sampleQuality) :
        mWidth(width), mHeight(height), mDepth(depth), mFormat(format), mDimension(dimension), mMipLevels(mipLevels),
        mSampleCount(sampleCount), mSampleQuality(sampleQuality) { }

    bool operator==(const RHITextureDesc& other) const noexcept
    {
        return memcmp(this, &other, sizeof(RHITextureDesc));
    }

    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mDepth;
    Format mFormat;
    ResourceDimension mDimension;
    uint8_t mMipLevels;
    uint8_t mSampleCount;
    uint8_t mSampleQuality;
private:
	uint8_t mPadding[3]{};
};

struct Viewport
{
    Viewport() = default;
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
    Rect() = default;
    Rect(int32_t left, int32_t top, int32_t right, int32_t bottom)
        : mLeft(left), mTop(top), mRight(right), mBottom(bottom) {}
    int32_t mLeft;
    int32_t mTop;
    int32_t mRight;
    int32_t mBottom;
};

struct TextureCopyLocation
{
    uint32_t mMipmap;
    uint32_t mArrayIndex;
    uint32_t mNumMipmapsOrArrayElems;
    uint32_t mPosX;
    uint32_t mPosY;
    uint32_t mPosZ;

    static TextureCopyLocation BufferLocation(uint64_t src)
    {
        TextureCopyLocation copy{};
        copy.mPosX = src;
        return copy;
    }

    static TextureCopyLocation Texture2DLocation(uint32_t baseMipmap, uint32_t posX, uint32_t posY)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = baseMipmap;
        copy.mPosX = posX;
        copy.mPosY = posY;
        return copy;
    }

    static TextureCopyLocation Texture2DLocation(uint32_t baseMipmap, uint32_t numMipmaps)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = baseMipmap;
        copy.mNumMipmapsOrArrayElems = numMipmaps;
        return copy;
    }

    static TextureCopyLocation TextureArrayLocation(uint32_t mipmap, uint32_t arrayIndex, uint32_t posX, uint32_t posY)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mArrayIndex = arrayIndex;
		copy.mNumMipmapsOrArrayElems = 1;
        copy.mPosX = posX;
        copy.mPosY = posY;
        return copy;
    }

    static TextureCopyLocation TextureArrayLocation(uint32_t mipmap, uint32_t baseArrayIndex, uint32_t numElems)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mArrayIndex = baseArrayIndex;
        copy.mNumMipmapsOrArrayElems = numElems;
        return copy;
    }

    static TextureCopyLocation Texture3DLocation(uint32_t mipmap, uint32_t posX, uint32_t posY, uint32_t posZ)
    {
        TextureCopyLocation copy{};
        copy.mMipmap = mipmap;
        copy.mPosX = posX;
        copy.mPosY = posY;
        copy.mPosZ = posZ;
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
    NONE	= 0,
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

enum class BlendType : bool
{
    LOGIC = false,
    COLOR = true,
};

enum class PrimitiveType : uint8_t
{
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
};

enum class ConservativeRasterization : uint8_t
{
    CONSERVATIVE_OFF,
    CONSERVATIVE_1,
    CONSERVATIVE_2,
    CONSERVATIVE_3,
    CONSERVATIVE_4,
};

struct RasterizerDesc
{
    DrawMode mDrawMode;
    CullMode mCullMode;
    DepthBiasSet mDepthBias;
    uint8_t mEnableMsaa;
    uint8_t mEnableConservativeMode;
    bool mFrontCounterClockWise;

    static RasterizerDesc Default()
    {
        return {DrawMode::SOLID, CullMode::BACK, DepthBiasSet::NONE, false, false};
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
        return {true, CompareFunction::GREATER_EQUAL, DepthOperation::WRITE, 0xff};
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
        return {true, 0xf, 0xf, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS };
    }

    static StencilTestDesc Disabled()
    {
        return { false, 0, 0, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS, StencilOperation::KEEP, StencilOperation::KEEP,
            StencilOperation::KEEP, CompareFunction::ALWAYS };
    }
};

struct BlendDesc
{
    bool mEnableBlend;
    BlendType mBlendType;
    BlendMode mSrcBlend;
    BlendMode mDestBlend;
    BlendOperation mBlendOp;
    BlendMode mSrcBlendAlpha;
    BlendMode mDestBlendAlpha;
    BlendOperation mBlendOpAlpha;
    LogicOperation mLogicOp;
    ColorMask mRenderTargetWriteMask;

    static BlendDesc Disabled()
    {
        return { false };
    }

    static BlendDesc Logic(LogicOperation logicOp, ColorMask colorMask)
    {
        return {true, BlendType::LOGIC, BlendMode::ZERO, BlendMode::ONE, BlendOperation::ADD, BlendMode::ZERO, BlendMode::ONE, BlendOperation::ADD, logicOp, colorMask};
    }

    static BlendDesc Color(BlendMode srcBlend = BlendMode::SRC_ALPHA, BlendMode destBlend = BlendMode::INV_SRC_ALPHA, BlendOperation blendOp = BlendOperation::ADD,
                                BlendMode srcBlendAlpha = BlendMode::ONE, BlendMode destBlendAlpha = BlendMode::ZERO, BlendOperation blendOpAlpha = BlendOperation::ADD,
                                ColorMask renderTargetWriteMask = ColorMask::ALL)
    {
        return { true, BlendType::COLOR, srcBlend, destBlend, blendOp, srcBlendAlpha, destBlendAlpha, blendOpAlpha, LogicOperation::CLEAR, renderTargetWriteMask};
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
    BlendDesc mBlendDesc;
    void* mVSShader;
    void* mHSShader;
    void* mDSShader;
    void* mGSShader;
    void* mPSShader;

    static GraphicPipelineStateDesc Default()
    {
        static GraphicPipelineStateDesc desc = {RasterizerDesc::Default(), Format::D24_UNORM_S8_UINT,
            {Format::R32G32B32A32_FLOAT},
            PrimitiveType::TRIANGLE_LIST,
            DepthTestDesc::Default(),
            StencilTestDesc::Default(),
            BlendDesc::Color(),
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            nullptr
            };
        return desc;
    } 
};

static uint16_t GetFormatStride(Format format)
{
    switch (format)
    {
    case Format::R8_UNORM:
    case Format::R8_SNORM:
    case Format::R8_UINT:
    case Format::R8_SINT:
        return 1; // 1 byte per pixel (8 bits)

    case Format::R8G8_UNORM:
    case Format::R8G8_SNORM:
    case Format::R8G8_UINT:
    case Format::R8G8_SINT:
    case Format::R16_UNORM:
    case Format::R16_SNORM:
    case Format::R16_UINT:
    case Format::R16_SINT:
    //case Format::R16_FLOAT:
        return 2; // 2 bytes per pixel (16 bits)

    case Format::R8G8B8A8_UNORM:
    case Format::R8G8B8A8_UNORM_SRGB:
    case Format::R8G8B8A8_SNORM:
    case Format::R8G8B8A8_UINT:
    case Format::R8G8B8A8_SINT:
    case Format::R16G16_UNORM:
    case Format::R16G16_SNORM:
    case Format::R16G16_UINT:
    case Format::R16G16_SINT:
    //case Format::R16G16_FLOAT:
    case Format::R32_TYPELESS:
    case Format::R32_UINT:
    case Format::R32_SINT:
    case Format::R32_FLOAT:
    case Format::D24_UNORM_S8_UINT:
        return 4; // 4 bytes per pixel (32 bits)

    case Format::R16G16B16A16_UNORM:
    case Format::R16G16B16A16_SNORM:
    case Format::R16G16B16A16_UINT:
    case Format::R16G16B16A16_SINT:
    //case Format::R16G16B16A16_FLOAT:
    case Format::R32G32_TYPELESS:
    case Format::R32G32_UINT:
    case Format::R32G32_SINT:
    case Format::R32G32_FLOAT:
        return 8; // 8 bytes per pixel (64 bits)

    //case Format::R32G32B32_TYPELESS:
    case Format::R32G32B32_UINT:
    case Format::R32G32B32_SINT:
    case Format::R32G32B32_FLOAT:
        return 12; // 12 bytes per pixel (96 bits)

    case Format::R32G32B32A32_TYPELESS:
    case Format::R32G32B32A32_UINT:
    case Format::R32G32B32A32_SINT:
    case Format::R32G32B32A32_FLOAT:
        return 16; // 16 bytes per pixel (128 bits)

    case Format::UNKNOWN:
    default:
        return 0; // Unknown format
    }
}
