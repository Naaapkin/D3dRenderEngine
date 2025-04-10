#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/Blob.h"

enum class VertexSegment : uint8_t
{
    POSITION,
    NORMAL,
    TANGENT,
    BITANGENT,
    COLOR,
    TEXCOORD
};

#undef DOMAIN
struct ShaderSourceCode final
{
    static ShaderSourceCode sLoadShader_Temp(const String& name, const char* pFileData, uint64_t size);
    static ShaderSourceCode sLoadShader(const String& name, const char* pFileData, uint64_t size);

    String mName;
    Blob mSource;
    std::string mVsEntry;
    std::string mHsEntry;
    std::string mDsEntry;
    std::string mGsEntry;
    std::string mPsEntry;
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

struct ShaderProperty
{
    String mName;
    uint32_t mRegister;
    ShaderPropType mType;
};

struct VertexInputElement
{
    std::string mSemanticName;
    uint32_t mSemanticIndex;
    DXGI_FORMAT mFormat;
    uint32_t mInputSlot;
};

class Shader : NonCopyable
{
    friend class std::hash<Shader>;
public:
    static std::vector<VertexInputElement> sBuildInputElements(const Shader& shader);
    static Shader* sCompile(const ShaderSourceCode& source);

    String Name() const;
    ShaderType Type() const;
    virtual const Blob& Binary() const = 0;

    Shader();
    Shader(const String& name, ShaderType type);
    virtual ~Shader();

    bool operator==(const Shader& other) const noexcept;
    
protected:
    static const std::vector<String>& sTempShaderManifest();

private:
    String mName;
    ShaderType mType;
    uint64_t mHashCache;
};

template<>
struct std::hash<Shader> {
    size_t operator()(const Shader& key) const noexcept
    {
        return key.mHashCache;
    }
};

inline ShaderSourceCode ShaderSourceCode::sLoadShader_Temp(const String& name, const char* pFileData,
                                                           uint64_t size)
{
    ShaderSourceCode shaderSourceCode {};
    shaderSourceCode.mVsEntry = new char[7] {"VsMain"};
    shaderSourceCode.mPsEntry = new char[7] {"PsMain"};
    shaderSourceCode.mSource = {pFileData, size};
    shaderSourceCode.mName = name;
    return shaderSourceCode;
}

inline ShaderSourceCode ShaderSourceCode::sLoadShader(const String& name, const char* pFileData,
                                                      uint64_t size)
{
    // TODO: resolve the shader source, get the names of entries; 
    ShaderSourceCode shaderSourceCode {};
    shaderSourceCode.mSource = {pFileData, size};
    shaderSourceCode.mName = name;
    return shaderSourceCode;
}

inline String Shader::Name() const
{
    return mName;
}

inline Shader::Shader() = default;

inline bool Shader::operator==(const Shader& other) const noexcept
{
    return mName == other.mName && mType == other.mType;
}

inline const std::vector<String>& Shader::sTempShaderManifest()
{
    static std::vector<String> shaderManifest{
        TEXT("opaque")
    };
    return shaderManifest;
}
#endif