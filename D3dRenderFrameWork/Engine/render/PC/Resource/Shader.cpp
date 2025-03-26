#ifdef WIN32
#include "Shader.h"

#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Core/D3dContext.h"
#include "Engine/render/PC/Core/D3dRenderer.h"

const std::vector<String>& Shader::sTempShaderManifest()
{
    static std::vector<String> shaderManifest{
        TEXT("opaque")
    };
    return shaderManifest;
}

ShaderSourceCode ShaderSourceCode::sLoadShader_Temp(const String& name, const char* pFileData,
                                                    uint64_t size)
{
    ShaderSourceCode shaderSourceCode {};
    char* data = new char[size + 1];
    memcpy(data, pFileData, size);
    shaderSourceCode.mVsEntry = new char[7] {"VsMain"};
    shaderSourceCode.mPsEntry = new char[7] {"PsMain"};
    shaderSourceCode.mSource = data;
    shaderSourceCode.mName = name;
    shaderSourceCode.mSourceSize = size;
    return shaderSourceCode;
}

ShaderSourceCode ShaderSourceCode::sLoadShader(const String& name, const char* pFileData,
                                               uint64_t size)
{
    // TODO: resolve the shader source, get the names of entries; 
    ShaderSourceCode shaderSourceCode {};
    char* data = new char[size + 1];
    memcpy(data, pFileData, size);
    shaderSourceCode.mSource = data;
    shaderSourceCode.mName = name;
    shaderSourceCode.mSourceSize = size;
    return shaderSourceCode;
}

ShaderSourceCode::ShaderSourceCode() = default;

ShaderSourceCode::~ShaderSourceCode() = default;

std::pair<uint8_t, bool> Shader::getPropertyIndex(const String& name)
{
    auto it = mShaderPropBindings.find(name);
    if (it == mShaderPropBindings.end()) return {0, false};
    return { it->second, true };
}

Shader Shader::sCompileShader(const ShaderSourceCode& src)
{
    Shader shader{ };
    auto&& name = ::Utf8ToAscii(src.mName);
    shader.mName = src.mName;
    shader.mHashCache = std::hash<String>{}(src.mName);
    if (!src.mVsEntry.empty()) shader.mVsBinary = sNativeCompile(name, src.mSource.c_str(), src.mSourceSize, src.mVsEntry, ShaderType::VERTEX);
    if (!src.mHsEntry.empty()) shader.mHsBinary = sNativeCompile(name, src.mSource.c_str(), src.mSourceSize, src.mHsEntry, ShaderType::HULL);
    if (!src.mDsEntry.empty()) shader.mDsBinary = sNativeCompile(name, src.mSource.c_str(), src.mSourceSize, src.mDsEntry, ShaderType::DOMAIN);
    if (!src.mGsEntry.empty()) shader.mGsBinary = sNativeCompile(name, src.mSource.c_str(), src.mSourceSize, src.mGsEntry, ShaderType::DEOMETRY);
    if (!src.mPsEntry.empty()) shader.mPsBinary = sNativeCompile(name, src.mSource.c_str(), src.mSourceSize, src.mPsEntry, ShaderType::PIXEL);
    shader.buildInputLayoutFootprint();
    return shader;
}

bool Shader::sBindShaderProps(ID3D12ShaderReflection* pReflector)
{
    D3D12_SHADER_DESC shaderDesc;
    D3D12_SHADER_INPUT_BIND_DESC bindingDesc;
    pReflector->GetDesc(&shaderDesc);
    for (uint32_t i = 0; i < shaderDesc.ConstantBuffers; ++i)
    {
        pReflector->GetResourceBindingDesc(i, &bindingDesc);
        String bufferName = AsciiToUtf8(bindingDesc.Name);
        mPropBindingReadMutex.lock_shared();
        auto it = mShaderPropBindings.find(bufferName);
        mPropBindingReadMutex.unlock_shared();
        if (it == mShaderPropBindings.end())
        {
            mPropBindingReadMutex.lock();
            mShaderPropBindings.emplace(bufferName, bindingDesc.BindPoint);
            mPropBindingReadMutex.unlock();
        }
        else if (it->second != bindingDesc.BindPoint)
        {
            DEBUG_WARN("conflict shader resource binding!");
        }
    }
    return true;
}

ID3DBlob* Shader::sNativeCompile(const std::string& name, const char* source, uint64_t size, const std::string& entry, ShaderType type)
{
    // TODO: replace this
    static constexpr char COMPILE_TARGETS[] = "vs_4_0\0hs_4_0\0ds_4_0\0gs_4_0\0ps_4_0";
    ID3DBlob* bin;
    ID3DBlob* error;
    if (FAILED(D3DCompile(source, size, name.c_str(), nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE, entry.c_str(), COMPILE_TARGETS + type * 7,
            D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR, 0, &bin, &error)))
    {
        OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        error->Release();
        return nullptr;
    }
    ID3D12ShaderReflection* pReflector;
    ThrowIfFailed(D3DReflect(bin->GetBufferPointer(), bin->GetBufferSize(), IID_PPV_ARGS(&pReflector)));
    if (sBindShaderProps(pReflector))
    {
        pReflector->Release();
        return bin;
    }
    DEBUG_WARN("there are conflicts in shader resource bindings");
    pReflector->Release();
    return nullptr;
}

Shader::Shader(Shader&& other) noexcept : mName(std::move(other.mName)), mHashCache(other.mHashCache),
    mVsBinary(other.mVsBinary), mHsBinary(other.mHsBinary), mDsBinary(other.mDsBinary), mGsBinary(other.mGsBinary), mPsBinary(other.mPsBinary),
    mInputLayout(other.mInputLayout), mVertexSize(other.mVertexSize)
{
    other.mVsBinary = nullptr;
    other.mHsBinary = nullptr;
    other.mDsBinary = nullptr;
    other.mGsBinary = nullptr;
    other.mPsBinary = nullptr;
    other.mInputLayout.NumElements = 0;
    other.mVertexSize = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this == &other)
    {
        mName = std::move(other.mName);
        mVsBinary = other.mVsBinary;
        mHsBinary = other.mHsBinary;
        mDsBinary = other.mDsBinary;
        mGsBinary = other.mGsBinary;
        mPsBinary = other.mPsBinary;
        mInputLayout = other.mInputLayout;
        mVertexSize = other.mVertexSize;
        
        other.mVsBinary = nullptr;
        other.mHsBinary = nullptr;
        other.mDsBinary = nullptr;
        other.mGsBinary = nullptr;
        other.mPsBinary = nullptr;
        other.mInputLayout.NumElements = 0;
        other.mVertexSize = 0;
    }
    return *this;
}

bool Shader::operator==(const Shader& other) const noexcept
{
    return this == &other || mName == other.mName;
}

const ID3DBlob* Shader::vsBinary() const
{
    return mVsBinary;
}

const ID3DBlob* Shader::hsBinary() const
{
    return mHsBinary;
}

const ID3DBlob* Shader::dsBinary() const
{
    return mDsBinary;
}

const ID3DBlob* Shader::gsBinary() const
{
    return mGsBinary;
}

const ID3DBlob* Shader::psBinary() const
{
    return mPsBinary;
}

D3D12_INPUT_LAYOUT_DESC Shader::inputLayout() const
{
    return mInputLayout;
}

uint32_t Shader::vertexSize() const
{
    return mVertexSize;
}

void Shader::buildInputLayoutFootprint()
{
    ID3D12ShaderReflection* pReflector;
    ThrowIfFailed(D3DReflect(mVsBinary->GetBufferPointer(), mVsBinary->GetBufferSize(), IID_PPV_ARGS(&pReflector)))
    D3D12_SHADER_DESC shaderDesc;
    pReflector->GetDesc(&shaderDesc);
    D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
    D3D12_INPUT_ELEMENT_DESC* inputElements = new D3D12_INPUT_ELEMENT_DESC[shaderDesc.InputParameters];
    uint32_t vertexSize = 0;
    for (uint32_t i = 0; i < shaderDesc.InputParameters; i++)
    {
        pReflector->GetInputParameterDesc(i, &inputDesc);

        inputElements[i] = {};
        uint32_t nameLength = strlen(inputDesc.SemanticName);
        char* name = new char[nameLength + 1];
        memcpy(name, inputDesc.SemanticName, nameLength);
        name[nameLength] = '\0';
        inputElements[i].SemanticName = name;
        inputElements[i].SemanticIndex = inputDesc.SemanticIndex;
        inputElements[i].InputSlot = 0;
        inputElements[i].Format = ::GetParaInfoFromSignature(inputDesc);
        inputElements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        inputElements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        inputElements[i].InstanceDataStepRate = 0;
        vertexSize += __popcnt(inputDesc.Mask);
    }
    mInputLayout.NumElements = shaderDesc.InputParameters;
    mInputLayout.pInputElementDescs = inputElements;
    mVertexSize = vertexSize * 4;
    pReflector->Release();
}

Shader::Shader() = default;

Shader::~Shader()
{
    if (mVsBinary) mVsBinary->Release();
    if (mHsBinary) mHsBinary->Release();
    if (mDsBinary) mDsBinary->Release();
    if (mGsBinary) mGsBinary->Release();
    if (mPsBinary) mPsBinary->Release();
    if (mInputLayout.pInputElementDescs)
    {
        for (int i = 0; i < mInputLayout.NumElements; ++i)
        {
            delete[] mInputLayout.pInputElementDescs->SemanticName;
        }
        delete[] mInputLayout.pInputElementDescs;
    }
}

std::unordered_map<String, uint8_t> Shader::mShaderPropBindings{};
std::shared_mutex Shader::mPropBindingReadMutex{};
#endif