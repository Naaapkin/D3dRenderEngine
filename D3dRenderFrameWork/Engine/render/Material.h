#pragma once
#ifdef WIN32
#include "RenderResource.h"
#include "Shader.h"
#include "Engine/pch.h"
struct ResourceHandle;

struct ShaderConstant
{
    ShaderConstant() = default;
    ShaderConstant(String name, uint64_t size) : mName(std::move(name)), mData(size){ }
    bool operator==(const ShaderConstant& other) const;
    
    String mName;
    Blob mData;
};

struct ShaderTexture
{
    ShaderTexture() : mHandle(MAXINT64) { }
    ShaderTexture(String name, ResourceHandle handle) : mName(std::move(name)), mHandle(handle){ }
    bool operator==(const ShaderTexture& other) const;
    
    String mName;
    ResourceHandle mHandle;
};

class Material
{
private:
    String mShaderName;
    std::vector<ShaderConstant> mConstants;
    std::vector<ShaderTexture> mTextures;
};

class MaterialInstance
{
public:
    Shader* shader;
    std::vector<Blob> mConstants;
    std::vector<ResourceHandle> mTextures;
};
#endif