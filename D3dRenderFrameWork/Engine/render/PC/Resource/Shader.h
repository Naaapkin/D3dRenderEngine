#pragma once
#ifdef WIN32
#include "Engine/pch.h"

class D3dContext;
struct GraphicSetting;

enum class VertexSegment : uint16_t
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
    friend class Shader;
    static ShaderSourceCode sLoadShader_Temp(const String& name, const char* pFileData, uint64_t size);
    static ShaderSourceCode sLoadShader(const String& name, const char* pFileData, uint64_t size);
    
    ~ShaderSourceCode();

    DEFAULT_COPY_CONSTRUCTOR(ShaderSourceCode);
    DEFAULT_MOVE_CONSTRUCTOR(ShaderSourceCode);
    DEFAULT_COPY_OPERATOR(ShaderSourceCode);
    DEFAULT_MOVE_OPERATOR(ShaderSourceCode);

private:
    ShaderSourceCode();
    
    String mName;
    std::string mSource;
    std::string mVsEntry;
    std::string mHsEntry;
    std::string mDsEntry;
    std::string mGsEntry;
    std::string mPsEntry;
    uint64_t mSourceSize;
};

class Shader
{
    friend class std::hash<Shader>;
    
public:
    static std::pair<uint8_t, bool> getPropertyIndex(const String& name);
    static Shader sCompileShader(const ShaderSourceCode& src);
    static bool sBindShaderProps(ID3D12ShaderReflection* pReflector);
    
    const ID3DBlob* vsBinary() const;
    const ID3DBlob* hsBinary() const;
    const ID3DBlob* dsBinary() const;
    const ID3DBlob* gsBinary() const;
    const ID3DBlob* psBinary() const;
    D3D12_INPUT_LAYOUT_DESC inputLayout() const;
    uint32_t vertexSize() const;
    Shader();
    Shader(Shader&& other) noexcept;
    ~Shader();
    
    Shader& operator=(Shader&& other) noexcept;
    bool operator==(const Shader& other) const noexcept;

    DELETE_COPY_CONSTRUCTOR(Shader)
    DELETE_COPY_OPERATOR(Shader)

private:
    enum ShaderType : uint8_t
    {
        VERTEX,
        HULL,
        DOMAIN,
        DEOMETRY,
        PIXEL
    };
    static const std::vector<String>& sTempShaderManifest();
    static ID3DBlob* sNativeCompile(const std::string& name, const char* source, uint64_t size, const std::string& entry, ShaderType type);

    void buildInputLayoutFootprint();
    
    static std::unordered_map<String, uint8_t> mShaderPropBindings;
    static std::shared_mutex mPropBindingReadMutex;

    String mName;
    uint64_t mHashCache;
    ID3DBlob* mVsBinary;
    ID3DBlob* mHsBinary; 
    ID3DBlob* mDsBinary;
    ID3DBlob* mGsBinary;
    ID3DBlob* mPsBinary;
    
    D3D12_INPUT_LAYOUT_DESC mInputLayout;
    uint32_t mVertexSize;
};

template<>
struct std::hash<Shader> {
    size_t operator()(const Shader& key) const noexcept
    {
        return key.mHashCache;
    }
};
#endif
