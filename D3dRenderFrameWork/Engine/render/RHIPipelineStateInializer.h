#pragma once
#include "Material.h"
#include "RHIDescriptors.h"
#include "Engine/pch.h"
#include "Engine/common/helper.h"

class RHIShader;

struct PipelineInitializer
{
    void SetShader(RHIShader* pShader)
    {
        mShader = pShader;
    }
    
    void SetConservativeMode(bool enable)
    {
        mOptions = enable ? mOptions | 0b000100 : mOptions & 0b111011;
    }

    void SetAntiAliasedLine(bool enable)
    {
		mOptions = enable ? mOptions | 0b000010 : mOptions & 0b111101;
    }

    void SetMSAA(uint8_t sampleCount, uint8_t sampleQuality)
    {
		mMSAA = sampleCount << 4 | sampleQuality;
    }
    
    void SetDepthTest(const DepthTestDesc& depthTest)
    {
        mDepthInitializer = 0;
        if (depthTest.mEnableDepthTest)
        {
            mOptions |= 0b100000;
            mDepthCompareFunction = depthTest.mCompareFunction;
            mDepthOperation = depthTest.mDepthOperation;
            mDepthWriteMask = depthTest.mDepthWriteMask;
        }
        else
        {
            mOptions &= 0b011111;
        }
    }

    void SetStencilTest(const StencilTestDesc& stencilTest)
    {
        mStencilInitializers[0] = 0;
        mStencilInitializers[1] = 0;
        if (stencilTest.mEnableStencilTest)
        {
            mOptions |= 0b010000;
            mStencilReadMask = stencilTest.mStencilReadMask;
            mStencilWriteMask = stencilTest.mStencilWriteMask; 
            mBackStencilFunc = stencilTest.mBackStencilFunc;
            mBackStencilFailOp = stencilTest.mBackStencilFailOp;
            mBackStencilDepthFailOp = stencilTest.mBackStencilDepthFailOp;
            mBackStencilPassOp = stencilTest.mBackStencilPassOp;
            mFrontStencilFunc = stencilTest.mFrontStencilFunc;
            mFrontStencilFailOp = stencilTest.mFrontStencilFailOp;
            mFrontStencilDepthFailOp = stencilTest.mFrontStencilDepthFailOp;
            mFrontStencilPassOp = stencilTest.mFrontStencilPassOp;
        }
        else
        {
            mOptions &= 0b101111;
        }
    }

    void SetDepthStencil(Format format)
    {
        mDepthStencil = format;
    }

    void SetRenderTarget(Format format)
    {
        mRenderTarget = format;
    }

    void SetRasterizer(const RasterizerDesc& rasterizer)
    {
        mCullMode = rasterizer.mCullMode;
        mDrawMode = rasterizer.mDrawMode;
        mDepthBias = rasterizer.mDepthBias;
        mFrontCounterClockWise = rasterizer.mFrontCounterClockWise;
    }

    void SetBlend(bool enableAlphaToCoverage, bool enableIndependentBlend, const BlendDesc& blendDesc)
    {
        /*ASSERT(numRenderTargets <= 8, TEXT("numRenderTargets out of range(0~7)"));
        SetNumRenderTarget(numRenderTargets);*/
        if (blendDesc.mEnableBlend)
        {
            mOptions |= 0b000001;
            mBlendOptions = enableIndependentBlend << 1 | enableAlphaToCoverage;
            mBlendType = blendDesc.mBlendType;
            mRenderTargetWriteMask = blendDesc.mRenderTargetWriteMask;
            if (blendDesc.mBlendType == BlendType::LOGIC)
            {
                mRenderTargetsBlend.mBlendInitializer = 0;
                mRenderTargetsBlend.mLogicOp = blendDesc.mLogicOp;
            }
            else
            {
                mRenderTargetsBlend.mSrcBlend = blendDesc.mSrcBlendAlpha;
                mRenderTargetsBlend.mDestBlend = blendDesc.mDestBlendAlpha;
                mRenderTargetsBlend.mBlendOp = blendDesc.mBlendOp;
                mRenderTargetsBlend.mSrcBlendAlpha = blendDesc.mSrcBlendAlpha;
                mRenderTargetsBlend.mDestBlendAlpha = blendDesc.mDestBlendAlpha;
                mRenderTargetsBlend.mBlendOpAlpha = blendDesc.mBlendOpAlpha;
            }
        }
        else
        {
            mOptions &= 0b111110;
            mBlendOptions = 0;
            mBlendType = BlendType::LOGIC;
            mRenderTargetWriteMask = ColorMask::ALL;
            mRenderTargetsBlend.mBlendInitializer = 0;
        }

        //for (uint8_t i = 0; i < numRenderTargets; i++)
        //{
        //    
        //}
        //mNumRenderTargets = numRenderTargets;
    }


    // use rtIndex == 255 to apply the format to all render targets
    void SetFrameBuffers(Format renderTargetFormat, Format depthStencilFormat)
    {
        mDepthStencil = depthStencilFormat;
        mRenderTarget = renderTargetFormat;
    }

    void SetDrawMode(DrawMode drawMode)
    {
        mDrawMode = drawMode;
    }

    //void SetNumRenderTarget(uint8_t numRenderTargets)
    //{
    //    const int64_t size = (mNumRenderTargets - numRenderTargets);
    //    if (size > 0)   // shrink
    //    {
	   //     uint64_t mask64 = (1 << ((numRenderTargets + 1) << 3)) - 1; // low (numRenderTargets * 8) bits are 1
    //    	uint8_t mask8 = (1 << (numRenderTargets + 1)) - 1;

    //    	uint64_t& renderTargetFormats = *reinterpret_cast<uint64_t*>(mRenderTarget);
    //    	uint64_t& renderTargetsWriteMasks = *reinterpret_cast<uint64_t*>(mRenderTargetWriteMask);
    //    	uint64_t& blendTypes = *reinterpret_cast<uint64_t*>(mBlendTypes);

    //    	mEnableBlends = mEnableBlends & mask8;
    //    	renderTargetFormats = renderTargetFormats & mask64;
    //    	blendTypes = blendTypes & mask64;
    //    	renderTargetsWriteMasks = renderTargetsWriteMasks & mask64;

    //    	memset(mRenderTargetsBlend + numRenderTargets, 0, sizeof(mRenderTargetsBlend) * size);

    //    }
    //    mNumRenderTargets = numRenderTargets;
    //}

    bool operator==(const PipelineInitializer& other) const
    {
        if (mShader != other.mShader || 
            mMSAA != other.mMSAA ||
            /*mNumRenderTarget != other.mNumRenderTargets ||*/
            mDepthStencil != other.mDepthStencil ||
            mRenderTarget != other.mRenderTarget ||
            mBlendOptions != other.mBlendOptions)
            return false;
        bool sameOptions = mOptions == other.mOptions;
        bool sameBlend = mBlendType == other.mBlendType && mRenderTargetWriteMask == other.mRenderTargetWriteMask;
        sameBlend &= mBlendType == BlendType::LOGIC ? mRenderTargetsBlend.mLogicOp == other.mRenderTargetsBlend.mLogicOp : mRenderTargetsBlend.mBlendInitializer == other.mRenderTargetsBlend.mBlendInitializer;
        
        if (!sameBlend || !sameOptions) return false;
        bool sameDepth = mOptions & 0b000001 ?  mDepthInitializer == other.mDepthInitializer : true;
        return sameDepth && (mOptions & 0b000010 ?  mStencilInitializers[0] == other.mStencilInitializers[0] && mStencilInitializers[1] == other.mStencilInitializers[1] : true);
    }

    uint64_t Hash() const
    {
        uint64_t hash = mOptions | (mMSAA << 8) | (static_cast<uint64_t>(mDepthStencil) << 16) |
            static_cast<uint64_t>(mRenderTarget) << 24 | static_cast<uint64_t>(mBlendOptions) << 32 |
            static_cast<uint64_t>(mBlendType) << 40 |
            static_cast<uint64_t>(mRenderTargetWriteMask) << 48;

        hash = MurmurHash(hash, mDepthInitializer << 32 | mRasterizerInitializer);
        hash = MurmurHash(hash, mStencilInitializers[0]);
        hash = MurmurHash(hash, mStencilInitializers[1]);
        hash = MurmurHash(hash, reinterpret_cast<uint64_t>(mShader));
        return hash;
    }

    void SetCullMode(CullMode cullMode)
    {
        mCullMode = cullMode;
    }

    // enable DepthTest, StencilTest, AntiAliasedLine, Blend

    static PipelineInitializer Default()
    {
        static PipelineInitializer initializer{
			0b00110011,         // enable DepthTest, StencilTest, AntiAliasedLine, Blend,
            0x10,
            Format::D24_UNORM_S8_UINT,
			Format::R8G8B8A8_UNORM,
            true,               // enable AlphaToCoverageEnable
            BlendType::COLOR,
            ColorMask::ALL,
            0,
            BlendMode::SRC_ALPHA,
        	BlendMode::INV_SRC_ALPHA,
        	BlendOperation::ADD,
        	BlendMode::ONE,
        	BlendMode::ZERO,
        	BlendOperation::ADD,
            0xff,
            0xff,
            {0},
			StencilOperation::KEEP,     // No Op if Stencil Test Failed
			StencilOperation::KEEP,     // No Op if Depth Test Failed
			StencilOperation::REPLACE,  // Replace if Stencil Test Passed
			CompareFunction::NEVER,     // Stencil Front Test Function
            StencilOperation::KEEP,
            StencilOperation::KEEP,
			StencilOperation::REPLACE,
			CompareFunction::NEVER,

			CompareFunction::GREATER,   // DepthCompareFunction
            DepthOperation::WRITE,      // Write if Passed Depth Test
            0xff, 
            {0},
            DrawMode::SOLID,
            CullMode::BACK,
            DepthBiasSet::NONE,
            true,
            nullptr
        };
        return initializer;
    }

    /*union
    {*/
        /*struct
        {
            bool mEnableDepthTest;
            bool mEnableStencilTest;
            bool mEnableMSAA;
            bool mEnableConservativeRasterization;
            bool mEnableAntiAliasedLine;
            bool mEnableBlend;
        };*/
    //};
    uint8_t mOptions;

    //uint8_t mMSAASampleCount;
    //uint8_t mMSAASampleQuality;
    //uint8_t mNumRenderTargets;
    uint8_t mMSAA;
    Format mDepthStencil;
    Format mRenderTarget;
    
    uint8_t mBlendOptions;

    BlendType mBlendType;
    ColorMask mRenderTargetWriteMask;
    uint8_t mPadding[1];
    //BlendType mBlendTypes[8];
    /*ColorMask mRenderTargetWriteMask[8];*/
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
        };
        uint64_t mBlendInitializer{};
        LogicOperation mLogicOp;
    } mRenderTargetsBlend/*[8]*/;

    union
    {
        struct
        {
            uint8_t mStencilReadMask;
            uint8_t mStencilWriteMask;
            uint8_t mStencilTestPadding[6];
            StencilOperation mFrontStencilFailOp;
            StencilOperation mFrontStencilDepthFailOp;
            StencilOperation mFrontStencilPassOp;
            CompareFunction mFrontStencilFunc;
            StencilOperation mBackStencilFailOp;
            StencilOperation mBackStencilDepthFailOp;
            StencilOperation mBackStencilPassOp;
            CompareFunction mBackStencilFunc;
        };
        uint64_t mStencilInitializers[2]{};
    };

    union
    {
        struct
        {
            CompareFunction mDepthCompareFunction;
            DepthOperation mDepthOperation;
            uint8_t mDepthWriteMask;
            uint8_t mDepthTestPadding[1];
        };
        uint32_t mDepthInitializer{};
    };

    union
    {
        struct
        {
            DrawMode mDrawMode;
            CullMode mCullMode;
            DepthBiasSet mDepthBias;
            uint8_t mFrontCounterClockWise;
        };
        uint32_t mRasterizerInitializer{};
    };

    RHIShader* mShader;

    PipelineInitializer() = default;
};

template <>
struct std::hash<PipelineInitializer>
{
    size_t operator()(const PipelineInitializer& key) const noexcept
    {
        return key.Hash();
    }
};