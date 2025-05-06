#pragma once
#ifdef WIN32
#include "RHIDescriptors.h"
#include "RenderResource.h"
#include "Shader.h"

struct ConstantProperty
{
    ConstantProperty() = default;
    ConstantProperty(const String& name, uint8_t registerSlot, uint64_t size) : mName(name),
        mRegisterSlot(registerSlot), mConstantSize(size)
    {
    }

	String mName;
    uint8_t mRegisterSlot;
    uint64_t mConstantSize;
};

struct TextureProperty
{
    TextureProperty() = default;
    TextureProperty(const String& name, uint64_t registerSlot, TextureDimension dimension) : mName(name),
        mRegisterSlot(registerSlot), mDimension(dimension)
    {
    }

    String mName;
    uint64_t mRegisterSlot;
    TextureDimension mDimension;
};

struct SamplerProperty  // TODO: complete SamplerProperty
{
	
};

// TODO: should be serializable
struct Material
{
    RHIShader* mShader = nullptr;
    std::unique_ptr<ConstantProperty[]> mConstants = nullptr;
    std::unique_ptr<TextureProperty[]> mTextures = nullptr;
    std::unique_ptr<SamplerProperty[]> mSamplers = nullptr;
    uint8_t mNumConstants = 0;
    uint8_t mNumTextures = 0;
    uint8_t mNumSamplers = 0;

    bool mEnableAlphaClip = false;
    CullMode mCullMode = CullMode::BACK;
    DrawMode mDrawMode = DrawMode::SOLID;
    DepthTestDesc mDepthTest;
    StencilTestDesc mStencilTest;
    BlendDesc mBlend;
};

class MaterialInstance
{
public:
    void InstantiateFrom(const Material* pMaterial);
    void SetMaterialInstanceId(uint64_t id);
    uint64_t GetMaterialInstanceId() const;
    RHIShader* GetShader() const;
    uint32_t GetCBufferIndex(const String& cbufferName) const;
    uint32_t GetTextureIndex(const String& name) const;
    //uint32_t GetSamplerIndex(const String& name) const;   // TODO: 

    void SetTexture(uint32_t index, TextureDimension dimension, TextureRef texture);
    void SetTexture(uint32_t index, TextureRef texture);
    TextureRef GetTexture(uint32_t index) const;
    const Blob& GetConstantBuffer(uint32_t index) const;
    uint8_t NumTextures() const;
    uint8_t NumConstantBuffers() const;

    bool AlphaClipEnabled() const;
    DrawMode GetDrawMode() const;
    CullMode GetCullMode() const;
    const DepthTestDesc& DepthTest() const;
    const StencilTestDesc& StencilTest() const;
    const BlendDesc& BlendMode() const;
    
    MaterialInstance() = default;
    
private:
    const Material* mMaterial;
    uint64_t mInstanceId;
    
    std::unique_ptr<Blob[]> mConstants;                
    std::unique_ptr<TextureRef[]> mTextures;   // since the CPU rarely changes texture content, we don't cache texture content.
	// std::unique_ptr<RHISamplerRef[]> mSamplers;   // TODO: implement samplers

    bool mEnableAlphaClip;
    DrawMode mDrawMode;
    CullMode mCullMode;
    DepthTestDesc mDepthTest;
    StencilTestDesc mStencilTest;
    BlendDesc mBlend;
};

inline void MaterialInstance::InstantiateFrom(const Material* pMaterial)
{
    mMaterial = pMaterial;
    mConstants.reset(new Blob[pMaterial->mNumConstants]);
    mTextures.reset(new TextureRef[pMaterial->mNumTextures]);
    for (size_t i = 0; i < pMaterial->mNumConstants; ++i)
    {
        mConstants[i].Reserve(pMaterial->mConstants[i].mConstantSize);
    }

    mEnableAlphaClip = pMaterial->mEnableAlphaClip;
    mCullMode = pMaterial->mCullMode;
    mDrawMode = pMaterial->mDrawMode;
    mDepthTest = pMaterial->mDepthTest;
    mStencilTest = pMaterial->mStencilTest;
    mBlend = pMaterial->mBlend;
}

inline void MaterialInstance::SetMaterialInstanceId(uint64_t id) { mInstanceId = id; }

inline uint64_t MaterialInstance::GetMaterialInstanceId() const { return mInstanceId; }

inline RHIShader* MaterialInstance::GetShader() const { return mMaterial->mShader; }

inline uint32_t MaterialInstance::GetCBufferIndex(const String& cbufferName) const
{
	for (uint32_t i = 0; i < mMaterial->mNumConstants; ++i)
	{
		if (mMaterial->mConstants[i].mName == cbufferName)
		{
			return i;
		}
	}
    return MAXUINT32;
}

inline uint32_t MaterialInstance::GetTextureIndex(const String& name) const
{
	for (uint32_t i = 0; i < mMaterial->mNumTextures; ++i)
	{
		if (mMaterial->mTextures[i].mName == name)
		{
			return i;
		}
	}
	return MAXUINT32;
}

inline void MaterialInstance::SetTexture(uint32_t index, TextureDimension dimension, TextureRef texture)
{
    if (index >= mMaterial->mNumTextures)
    {
        WARN("index of material texture out of bound!");
        return;
    }
    if (dimension != mMaterial->mTextures[index].mDimension)
    {
        WARN("dimension of material texture mismatch!");
        return;
    }
    mTextures[index] = texture;
}

inline void MaterialInstance::SetTexture(uint32_t index, TextureRef texture)
{
    if (index >= mMaterial->mNumTextures)
    {
        WARN("index of material texture out of bound!");
        return;
    }
    mTextures[index] = texture;
}

inline TextureRef MaterialInstance::GetTexture(uint32_t index) const
{
    if (index >= mMaterial->mNumTextures)
    {
        WARN("index of material texture out of bound!");
        return MAXUINT64;
    }
    return mTextures[index];
}

inline const Blob& MaterialInstance::GetConstantBuffer(uint32_t index) const
{
    if (index >= mMaterial->mNumConstants)
    {
        WARN("index of material constants out of bound!");
    }
    return mConstants[index];
}

inline uint8_t MaterialInstance::NumTextures() const
{
    return mMaterial->mNumTextures;
}

inline uint8_t MaterialInstance::NumConstantBuffers() const
{
    return mMaterial->mNumConstants;
}

inline bool MaterialInstance::AlphaClipEnabled() const
{
    return mEnableAlphaClip;
}

inline DrawMode MaterialInstance::GetDrawMode() const
{
    return mDrawMode;
}

inline CullMode MaterialInstance::GetCullMode() const
{
    return mCullMode;
}

inline const DepthTestDesc& MaterialInstance::DepthTest() const
{
    return mDepthTest;
}

inline const StencilTestDesc& MaterialInstance::StencilTest() const
{
    return mStencilTest;
}

inline const BlendDesc& MaterialInstance::BlendMode() const
{
    return mBlend;
}
#endif
