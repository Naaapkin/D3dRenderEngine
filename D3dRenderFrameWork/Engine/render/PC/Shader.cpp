#ifdef WIN32
#include <Engine/common/helper.h>
#include <Engine/common/Exception.h>
#include <Engine/render/PC/D3dUtil.h>
#include <Engine/render/PC/Data.h>
#include <Engine/render/PC/Shader.h>

void Shader::LoadBinary(const String& path, ID3DBlob** bin)
{
    std::ifstream fIn{ path, std::ios::binary };
    if (!fIn.is_open()){
        // TODO: warn
        return;
    }
    fIn.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = fIn.tellg();
    fIn.seekg(0, std::ios_base::beg);
    ThrowIfFailed(D3DCreateBlob(size, bin));
    fIn.read(static_cast<char*>((*bin)->GetBufferPointer()), size);
    fIn.close();
}

void Shader::LoadAndCompile(const std::wstring& path, ID3DBlob** bin, ShaderType type, ID3DBlob** error)
{
    LoadAndCompile(Utf8ToAscii(path), bin, type, error);
}

void Shader::LoadAndCompile(const std::string& path, ID3DBlob** bin, ShaderType type, ID3DBlob** error)
{
    std::ifstream fIn{ path, std::ios::binary };
#ifdef DEBUG
    ASSERT(fIn.is_open(), TEXT("Failed to open shader file"));
#else
    // TODO: warning
#endif
    fIn.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = fIn.tellg();
    fIn.seekg(0, std::ios_base::beg);
    char* buffer = new char[size];
    fIn.read(buffer, size);
    fIn.close();

    char* shaderModel = new char[6];
    memcpy(shaderModel, SHADER_TYPE_STR + static_cast<uint64_t>(type) * 3, 3);
    memcpy(shaderModel + 3, DEFAULT_ENTRY_POINT, 3);
    D3DCompile(buffer, size,
        ::GetFileNameFromPath(path).c_str(), nullptr,
        nullptr, DEFAULT_ENTRY_POINT, shaderModel,
        0, 0,
        bin, error);
}

Shader Shader::LoadCompiledShader(const String& path, ShaderType type)
{
    ID3DBlob* bin;
    LoadBinary(path, &bin);
    ID3D12ShaderReflection* pReflector;
    ThrowIfFailed(D3DReflect(bin, bin->GetBufferSize(), IID_PPV_ARGS(&pReflector)))
    return CreateShader(bin, pReflector, type);
}

void Shader::GetShaderParameters(ShaderParameter const** pParameters, uint8_t& numParameters) const
{
    *pParameters = mInputParameters;
    numParameters = mNumInputParameters;
}

const ID3DBlob* Shader::Binary() const
{
    return mBinary;
}

ShaderType Shader::Type() const
{
    return mType;
}

Shader::~Shader()
{
    mBinary->Release();
    delete[] mInputParameters;
}

Shader::Shader(ID3DBlob* pBinary, ShaderType type, const ShaderParameter* shaderParameters,
    uint8_t numShaderParameters) :
    mBinary(pBinary), mType(type),
    mInputParameters(shaderParameters), mNumInputParameters(numShaderParameters) { }

Shader CreateShader(ID3DBlob* bin, ID3D12ShaderReflection* pReflector, ShaderType type)
{
    D3D12_SHADER_DESC shaderDesc;
    pReflector->GetDesc(&shaderDesc);
    uint8_t numParameter = shaderDesc.InputParameters;
    ShaderParameter* parameters = new ShaderParameter[numParameter];
    D3D12_SIGNATURE_PARAMETER_DESC inputDesc;
    for (int i = 0; i < numParameter; i++)
    {
        pReflector->GetInputParameterDesc(i, &inputDesc);
        
        parameters[i].mSemantic = inputDesc.SemanticName;
        parameters[i].mSemanticIndex = inputDesc.SemanticIndex;
        parameters[i].mInputSlot = inputDesc.Register;
        parameters[i].mFormat = ::GetParaFormatFromSignature(inputDesc); 
    }
    return { bin, type, parameters, numParameter };
}

std::vector<String> Shader::sShaderIncludes{2}; // 2 : expected number of includes, current: "common/Transform.hlsl", "common/LightConstants.hlsl"
#endif