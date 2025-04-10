#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/Shader.h"

struct ShaderReflector;
class HlslCompiler;

class HlslShader final : public Shader
{
    friend class HlslCompiler;
    
public:
    const Blob& vsBinary() const override;
    const Blob& hsBinary() const override;
    const Blob& dsBinary() const override;
    const Blob& gsBinary() const override;
    const Blob& psBinary() const override;
    HlslShader();
    HlslShader(const String& name);
    HlslShader(HlslShader&& other) noexcept;
    ~HlslShader() override;
    
    HlslShader& operator=(HlslShader&& other) noexcept;
    bool operator==(const HlslShader& other) const noexcept;

    DELETE_COPY_CONSTRUCTOR(HlslShader)
    DELETE_COPY_OPERATOR(HlslShader)

private:
    static void sGetProperties(ID3D12ShaderReflection* pReflector, std::vector<ShaderProperty>& properties);
    
    Blob mVsBinary;
    Blob mHsBinary; 
    Blob mDsBinary;
    Blob mGsBinary;
    Blob mPsBinary;
};

struct ShaderReflector
{
    ShaderReflector();
    ShaderReflector(ShaderReflector&& other) noexcept;
    ~ShaderReflector();
    ShaderReflector& operator=(ShaderReflector&& other) noexcept;

    DELETE_COPY_CONSTRUCTOR(ShaderReflector);
    DELETE_COPY_OPERATOR(ShaderReflector);
    
    ID3D12ShaderReflection* mVsReflector;
    ID3D12ShaderReflection* mHsReflector;
    ID3D12ShaderReflection* mDsReflector;
    ID3D12ShaderReflection* mGsReflector;
    ID3D12ShaderReflection* mPsReflector;

    std::vector<ShaderProperty> mVsProperties;
    std::vector<ShaderProperty> mHsProperties;
    std::vector<ShaderProperty> mDsProperties;
    std::vector<ShaderProperty> mGsProperties;
    std::vector<ShaderProperty> mPsProperties;
};

class HlslCompiler final : public ShaderCompiler
{
    friend class ShaderCompiler;
    
public:
    void compileShader(const ShaderSourceCode& sourceCode, Shader& shader) override;

private:
    static Blob sCompileImpl(const std::string& name, const Blob& blob, const std::string& entry, const char* pTarget);
    static HlslCompiler* sCompiler;
};
#endif
