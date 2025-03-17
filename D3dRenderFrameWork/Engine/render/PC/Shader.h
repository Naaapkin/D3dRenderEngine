#pragma once
#ifdef WIN32
#include <pch.h>
#include <Engine/render/PC/Data.h>

struct ShaderParameter;

enum class VertexSegment : uint16_t
{
    POSITION,
    NORMAL,
    TANGENT,
    BITANGENT,
    COLOR,
    TEXCOORD
};

enum class ShaderType
{
    VERTEX_SHADER,
    HULL_SHADER,
    DOMAIN_SHADER,
    GEOMETRY_SHADER,
    PIXEL_SHADER,
};

class Shader
{
    friend Shader CreateShader(ID3DBlob* bin, ID3D12ShaderReflection* pReflector, ShaderType type);
    
public:
    static Shader LoadCompiledShader(const String& path, ShaderType type);

    void GetShaderParameters(ShaderParameter const** pParameters, uint8_t& numParameters) const;
    const ID3DBlob* Binary() const;
    ShaderType Type() const;
    ~Shader();

    DELETE_COPY_CONSTRUCTOR(Shader)
    DELETE_COPY_OPERATOR(Shader)
    DEFAULT_MOVE_CONSTRUCTOR(Shader)
    DEFAULT_MOVE_OPERATOR(Shader)

private:
    static void LoadBinary(const String& path, ID3DBlob** bin);
    static void LoadAndCompile(const std::wstring& path, ID3DBlob** bin, ShaderType type, ID3DBlob** error = nullptr);
    static void LoadAndCompile(const std::string& path, ID3DBlob** bin, ShaderType type, ID3DBlob** error = nullptr);
    
    Shader(ID3DBlob* pBinary, ShaderType type, const ShaderParameter* shaderParameters, uint8_t numShaderParameters);

    constexpr static char DEFAULT_ENTRY_POINT[] = "main";
    constexpr static char DEFAULT_SHADER_MODEL[] = "6_0";
    constexpr static char SHADER_TYPE_STR[] = "vs_hs_ds_gs_ps_";

    ID3DBlob* mBinary;
    ShaderType mType;
    const ShaderParameter* mInputParameters;
    uint8_t mNumInputParameters;
};

Shader CreateShader(ID3DBlob* bin, ID3D12ShaderReflection* pReflector, ShaderType type);
#endif
