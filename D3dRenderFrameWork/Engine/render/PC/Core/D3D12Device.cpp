#include "D3D12Device.h"

ID3D12Resource* D3D12Device::CreateCommitedResource(const D3D12_HEAP_PROPERTIES& heapProp,
    const D3D12_HEAP_FLAGS& heapFlags, const D3D12_RESOURCE_DESC& desc, const D3D12_CLEAR_VALUE& clearValue,
    D3D12_RESOURCE_STATES initialState) const
{
    ID3D12Resource* pResource;
    mDevice->CreateCommittedResource(&heapProp, heapFlags, &desc, initialState, &clearValue, IID_PPV_ARGS(&pResource));
    return pResource;
}

void D3D12Device::CreatePipelineStateObject(GraphicPipelineStateDesc psoDesc)
{
    ASSERT(psoDesc.mShader, TEXT("shader is null"));;
    ASSERT(psoDesc.mRootSignature, TEXT("root signature is null"));
    ASSERT(psoDesc.mRenderTargetFormats, TEXT("render target format is null"));
        
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = psoDesc.mRootSignature;
    desc.VS = {psoDesc.mShader->vsBinary().binary(), psoDesc.mShader->vsBinary().size() * sizeof(byte) };
    desc.HS = {psoDesc.mShader->hsBinary().binary(), psoDesc.mShader->hsBinary().size() * sizeof(byte) };
    desc.DS = {psoDesc.mShader->dsBinary().binary(), psoDesc.mShader->dsBinary().size() * sizeof(byte) };
    desc.GS = {psoDesc.mShader->gsBinary().binary(), psoDesc.mShader->gsBinary().size() * sizeof(byte) };
    desc.PS = {psoDesc.mShader->psBinary().binary(), psoDesc.mShader->psBinary().size() * sizeof(byte) };
    const auto& inputElems = Shader::sBuildInputElements(*psoDesc.mShader);
    D3D12_INPUT_ELEMENT_DESC* d3dInputElems = new D3D12_INPUT_ELEMENT_DESC[inputElems.size()];
    for (int i = 0; i < inputElems.size(); ++i)
    {
        d3dInputElems[i].SemanticName = inputElems[i].mSemanticName.c_str();
        d3dInputElems[i].SemanticIndex = inputElems[i].mSemanticIndex;
        d3dInputElems[i].InputSlot = inputElems[i].mInputSlot;
        d3dInputElems[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        d3dInputElems[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        d3dInputElems[i].InstanceDataStepRate = 0;
    }
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
    inputLayoutDesc.NumElements = inputElems.size();
    inputLayoutDesc.pInputElementDescs = d3dInputElems;
    desc.InputLayout = inputLayoutDesc;
    desc.Flags = psoDesc.mFlags;
    desc.SampleDesc = psoDesc.mSampleDesc;

    desc.DSVFormat = static_cast<DXGI_FORMAT>(psoDesc.mDepthStencilFormat);
    for (int i = 0; i < psoDesc.mNumRenderTargetFormats; ++i)
    {
        desc.RTVFormats[i] = static_cast<DXGI_FORMAT>(psoDesc.mRenderTargetFormats[i]);
    }
    desc.BlendState = psoDesc.mBlendDesc;
    desc.RasterizerState = psoDesc.mRasterizerDesc;
    desc.NumRenderTargets = psoDesc.mNumRenderTargetFormats;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    desc.NodeMask = 0;
        
    ID3D12PipelineState* pipelineState;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState)));
    mPsoCache.insert(std::pair<Shader*, ID3D12PipelineState*>(psoDesc.mShader, pipelineState));
        
}

ID3D12DescriptorHeap* D3D12Device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
    D3D12_DESCRIPTOR_HEAP_FLAGS flag, uint32_t numDescriptors) const
{
    ID3D12DescriptorHeap* pHeap;
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Flags = flag;
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.NodeMask = 0;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pHeap)));
    return pHeap;
}

ID3D12RootSignature* D3D12Device::CreateRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const
{
    // std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges1;
    // std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges2;
    // std::vector<CD3DX12_ROOT_PARAMETER1> rootParams;
    //
    // // TODO: remove magic number
    // ranges1.resize(1);
    // ranges2.resize(1);
    // rootParams.resize(2);
    // ranges1[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GraphicSetting::gNumPassConstants, 0);
    // rootParams[0].InitAsDescriptorTable(ranges1.size(), ranges1.data());
    // ranges2[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, GraphicSetting::gNumPerObjectConstants, GraphicSetting::gNumPassConstants);
    // rootParams[1].InitAsDescriptorTable(ranges2.size(), ranges2.data());
    //
    // auto&& staticSamplers = GetStaticSamplers();
    // CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
    // desc.Init_1_1(rootParams.size(), rootParams.data(),
    //     staticSamplers.size(), staticSamplers.data(),
    //     D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    
    ID3D12RootSignature* pRootSignature;
    UComPtr<ID3DBlob> blob;
    UComPtr<ID3DBlob> errorBlob;
    if (FAILED((D3D12SerializeVersionedRootSignature(&desc, blob.GetAddressOf(), errorBlob.GetAddressOf()))))
    {
        OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
        THROW_EXCEPTION(TEXT("Failed to create root signature"));
    }
    ThrowIfFailed(mDevice->CreateRootSignature(0, blob->GetBufferPointer(),
        blob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)));
    return pRootSignature;
}

ID3D12CommandQueue* D3D12Device::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& queueDesc) const
{
    ID3D12CommandQueue* pCommandQueue;
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCommandQueue)));
    return pCommandQueue;
}

D3D12Device::D3D12Device(ID3D12Device* device): mDevice(device)
{ }

std::vector<D3D12_STATIC_SAMPLER_DESC> D3D12Device::GetStaticSamplers()
{
    return {
        // 点采样器（用于像素精确采样）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_MIN_MAG_MIP_POINT,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
             0.0f,
            D3D12_FLOAT32_MAX,
            0,  // 绑定到 s0
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 线性采样器（用于平滑插值）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            0.0f,
            D3D12_FLOAT32_MAX,
            1,  // 绑定到 s1
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 线性采样器（用于平滑插值）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            1,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
            0.0f,
            D3D12_FLOAT32_MAX,
            2,  // 绑定到 s2
            0,
            D3D12_SHADER_VISIBILITY_PIXEL
        },

        // 各向异性采样器（用于高质量纹理）
        D3D12_STATIC_SAMPLER_DESC{
            D3D12_FILTER_ANISOTROPIC,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0.0f,
            16,
            D3D12_COMPARISON_FUNC_ALWAYS,
            D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
            0.0f,
            D3D12_FLOAT32_MAX,
            3,  // 绑定到 s3
            0,
            D3D12_SHADER_VISIBILITY_ALL
        }
    };
}
