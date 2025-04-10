#ifdef WIN32
#include "HlslShader.h"

#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"
#include "Engine/render/Shader.h"
#include "Engine/render/PC/D3dUtil.h"

Shader* sCompile(const ShaderSourceCode& source)
{
    HlslShader* shader = new HlslShader{source.mName};
    HlslCompiler::getOrInitializeCompiler().compileShader(source, *shader);
    return shader;
}

ShaderReflector& ShaderCompiler::getReflector(const String& name)
{
    return mReflectors[name];
}

ShaderReflector::ShaderReflector() = default;

ShaderReflector::ShaderReflector(ShaderReflector&& other) noexcept : mVsReflector(other.mVsReflector),
                                                                     mHsReflector(other.mHsReflector),
                                                                     mDsReflector(other.mDsReflector),
                                                                     mGsReflector(other.mGsReflector),
                                                                     mPsReflector(other.mPsReflector)
{
    other.mVsReflector = nullptr;
    other.mHsReflector = nullptr;
    other.mDsReflector = nullptr;
    other.mGsReflector = nullptr;
    other.mPsReflector = nullptr;
}

ShaderReflector::~ShaderReflector()
{
    if (mVsReflector) mVsReflector->Release();
    if (mHsReflector) mHsReflector->Release();
    if (mDsReflector) mDsReflector->Release();
    if (mGsReflector) mGsReflector->Release();
    if (mPsReflector) mPsReflector->Release();
}

ShaderReflector& ShaderReflector::operator=(ShaderReflector&& other) noexcept
{
    if (this != &other)
    {
        mVsReflector = other.mVsReflector;
        mHsReflector = other.mHsReflector;
        mDsReflector = other.mDsReflector;
        mGsReflector = other.mGsReflector;
        mPsReflector = other.mPsReflector;
        
        other.mVsReflector = nullptr;
        other.mHsReflector = nullptr;
        other.mDsReflector = nullptr;
        other.mGsReflector = nullptr;
        other.mPsReflector = nullptr;
    }
    return *this;
}

HlslShader::HlslShader(HlslShader&& other) noexcept : Shader(std::move(other)),
                                                      mVsBinary(std::move(other.mVsBinary)), mHsBinary(std::move(other.mHsBinary)), mDsBinary(std::move(other.mDsBinary)),
                                                      mGsBinary(std::move(other.mGsBinary)), mPsBinary(std::move(other.mPsBinary)) { }

HlslShader& HlslShader::operator=(HlslShader&& other) noexcept
{
    if (this == &other)
    {
        Shader::operator=(std::move(other));

        mVsBinary = std::move(other.mVsBinary);
        mHsBinary = std::move(other.mHsBinary);
        mDsBinary = std::move(other.mDsBinary);
        mGsBinary = std::move(other.mGsBinary);
        mPsBinary = std::move(other.mPsBinary);
    }
    return *this;
}

bool HlslShader::operator==(const HlslShader& other) const noexcept
{
    return this == &other || Name() == other.Name();
}

void HlslShader::sGetProperties(ID3D12ShaderReflection* pReflector, std::vector<ShaderProperty>& properties)
{
    D3D12_SHADER_DESC shaderDesc;
    D3D12_SHADER_INPUT_BIND_DESC bindingDesc;
    pReflector->GetDesc(&shaderDesc);
    for (uint32_t i = 0; i < shaderDesc.BoundResources; ++i)
    {
        pReflector->GetResourceBindingDesc(i, &bindingDesc);
        ShaderProperty property;
        property.mRegister = bindingDesc.BindPoint;
#ifdef UNICODE
        property.mName = ::AsciiToUtf8(bindingDesc.Name);
#else
        property.mName = bindingDesc.Name;
#endif
        switch (bindingDesc.Type)
        {
        case D3D_SIT_CBUFFER:
            property.mType = ShaderPropType::CBUFFER;
            break;
        case D3D_SIT_TEXTURE:
        case D3D_SIT_TBUFFER:
            property.mType = ShaderPropType::TEXTURE;
            break;
        case D3D_SIT_SAMPLER:
            property.mType = ShaderPropType::SAMPLER;
            break;
        default:
            break;
        }
        properties.emplace_back(std::move(property));
    }
}

std::vector<VertexInputElement> Shader::sBuildInputElements(const Shader& shader)
{
    if (shader.vsBinary().binary())
    {
        WARN("failed to build input elements: shader has no vertex input.")
        return {};
    }
    ID3D12ShaderReflection* pReflector;
    ThrowIfFailed(D3DReflect(shader.vsBinary().binary(), shader.vsBinary().size(), IID_PPV_ARGS(&pReflector)))
    D3D12_SHADER_DESC shaderDesc;
    pReflector->GetDesc(&shaderDesc);
    D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
    std::vector<VertexInputElement> inputElements;
    inputElements.resize(shaderDesc.InputParameters);
    for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
    {
        pReflector->GetInputParameterDesc(i, &inputDesc);

        inputElements[i] = {};
        inputElements[i].mSemanticName = inputDesc.SemanticName;
        inputElements[i].mSemanticIndex = inputDesc.SemanticIndex;
        inputElements[i].mInputSlot = 0;
        inputElements[i].mFormat = ::GetParaInfoFromSignature(inputDesc);
    }
    pReflector->Release();
    return inputElements;
}

ShaderType Shader::Type() const
{
    return mType;
}

Shader::Shader(const String& name, ShaderType type) : mName(name), mType(type)
{
    mHashCache = std::hash<String>()(name) ^ std::hash<ShaderType>()(type);
}

Shader::~Shader() = default;

const Blob& HlslShader::vsBinary() const
{
    return mVsBinary;
}

const Blob& HlslShader::hsBinary() const
{
    return mHsBinary;
}

const Blob& HlslShader::dsBinary() const
{
    return mDsBinary;
}

const Blob& HlslShader::gsBinary() const
{
    return mGsBinary;
}

const Blob& HlslShader::psBinary() const
{
    return mPsBinary;
}

HlslShader::HlslShader() = default;

HlslShader::HlslShader(const String& name) : Shader(name) { }

HlslShader::~HlslShader()
{
    mVsBinary.release();
    mHsBinary.release();
    mDsBinary.release();
    mGsBinary.release();
    mPsBinary.release();
}

ShaderCompiler& ShaderCompiler::getOrInitializeCompiler()
{
    if (!HlslCompiler::sCompiler) HlslCompiler::sCompiler = new HlslCompiler();
    return *HlslCompiler::sCompiler;
}

void HlslCompiler::compileShader(const ShaderSourceCode& sourceCode, Shader& shader)
{
    HlslShader& hlslShader = static_cast<HlslShader&>(shader);
    const auto it = mReflectors.find(sourceCode.mName);
    if (it != mReflectors.end())
    {
        WARN("shader has already compiled with the same name")
        return;
    }
    auto&& name = ::Utf8ToAscii(sourceCode.mName);
    ShaderReflector reflector;
    if (!sourceCode.mVsEntry.empty())
    {
        hlslShader.mVsBinary = std::move(sCompileImpl(name, sourceCode.mSource, sourceCode.mVsEntry, "vs_4_0"));
        ThrowIfFailed(D3DReflect(shader.vsBinary().binary(), shader.vsBinary().size(), IID_PPV_ARGS(&reflector.mVsReflector)));
        HlslShader::sGetProperties(reflector.mVsReflector, reflector.mVsProperties);
    }
    if (!sourceCode.mHsEntry.empty())
    {
        hlslShader.mHsBinary = std::move(sCompileImpl(name, sourceCode.mSource, sourceCode.mHsEntry, "hs_4_0"));
        ThrowIfFailed(D3DReflect(shader.hsBinary().binary(), shader.hsBinary().size(), IID_PPV_ARGS(&reflector.mHsReflector)));
        HlslShader::sGetProperties(reflector.mHsReflector, reflector.mHsProperties);
    }
    if (!sourceCode.mDsEntry.empty())
    {
        hlslShader.mDsBinary = std::move(sCompileImpl(name, sourceCode.mSource, sourceCode.mDsEntry, "ds_4_0"));
        ThrowIfFailed(D3DReflect(shader.dsBinary().binary(), shader.dsBinary().size(), IID_PPV_ARGS(&reflector.mDsReflector)));
        HlslShader::sGetProperties(reflector.mDsReflector, reflector.mDsProperties);
    }
    if (!sourceCode.mGsEntry.empty())
    {
        hlslShader.mGsBinary = std::move(sCompileImpl(name, sourceCode.mSource, sourceCode.mGsEntry, "gs_4_0"));
        ThrowIfFailed(D3DReflect(shader.gsBinary().binary(), shader.gsBinary().size(), IID_PPV_ARGS(&reflector.mGsReflector)));
        HlslShader::sGetProperties(reflector.mGsReflector, reflector.mGsProperties);
    }
    if (!sourceCode.mPsEntry.empty())
    {
        hlslShader.mPsBinary = std::move(sCompileImpl(name, sourceCode.mSource, sourceCode.mPsEntry, "ps_4_0"));
        ThrowIfFailed(D3DReflect(shader.psBinary().binary(), shader.psBinary().size(), IID_PPV_ARGS(&reflector.mPsReflector)));
        HlslShader::sGetProperties(reflector.mPsReflector, reflector.mPsProperties);
    }
    mReflectors.emplace(sourceCode.mName, std::move(reflector));
}

Blob HlslCompiler::sCompileImpl(const std::string& name, const Blob& blob, const std::string& entry,
                                  const char* pTarget)
{
    ComPtr<ID3DBlob> bin;
    ID3DBlob* error;
    if (FAILED(D3DCompile(blob.binary(), blob.size(), name.c_str(), nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE, entry.c_str(), pTarget,
            D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR, 0, &bin, &error)))
    {
        OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        error->Release();
        return {};
    }
    return {bin->GetBufferPointer(), bin->GetBufferSize()};
}
#endif