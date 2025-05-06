#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/RHIPipelineStateInializer.h"
#include "D3D12Device.h"
#include "Engine/render/PC/D3dUtil.h"
#include "D3D12EnumConversions.h"
#include "Engine/render/Blob.h"
#include "Engine/render/Shader.h"

enum class D3D12PipelineStateType : uint8_t
{
    GRAPHIC,
    COMPUTE,
};

class D3D12PipelineStateManager final : NonCopyable
{
public:
    static void InitD3D12PipelineStateDesc(ID3D12RootSignature* pRootSignature, const PipelineInitializer& psoDesc, D3D12_GRAPHICS_PIPELINE_STATE_DESC& d3d12Desc);
    
    void Initialize(D3D12Device* pDevice);
    ID3D12PipelineState* GetOrCreateGraphicsPSO(ID3D12RootSignature* pRootSignature,
                                                const PipelineInitializer& desc);

    // UComPtr<ID3D12PipelineState> GetOrCreateComputePSO(ID3D12RootSignature* pRootSignature,
    //                                                     const PipelineInitializer& desc);
    D3D12PipelineStateManager() = default;
    ~D3D12PipelineStateManager() = default;

private:
    D3D12Device* mDevice;
    std::vector<PipelineInitializer> mPSOInitializers;      // TODO: monitor PSOs, Release unused PSOs when necessary.
    std::unordered_map<const PipelineInitializer*, UComPtr<ID3D12PipelineState>, HashPtrAsTyped<const PipelineInitializer*>, ComparePtrAsTyped<const PipelineInitializer*>> mPipelineStateTable;
};

inline void D3D12PipelineStateManager::Initialize(D3D12Device* pDevice)
{
    mDevice = pDevice;
}

inline ID3D12PipelineState* D3D12PipelineStateManager::GetOrCreateGraphicsPSO(
    ID3D12RootSignature* pRootSignature, const PipelineInitializer& desc)
{
    const auto it = mPipelineStateTable.find(&desc);
    if (it != mPipelineStateTable.end()) return it->second.Get();
    uint64_t index = mPSOInitializers.size();
    mPSOInitializers.push_back(desc);
    // if (mReleasedPSOs.empty())
    // {
    //     index = mReleasedPSOs.top();
    //     mReleasedPSOs.pop();
    //     mPSOInitializers[index] = desc;
    // }
    // else
    // {
    //     index = mPSOInitializers.mSize();
    //     mPSOInitializers.push_back(desc);
    // }
    D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc = {};
    InitD3D12PipelineStateDesc(pRootSignature, desc, d3d12Desc);
    ID3D12PipelineState* pPSO = mDevice->CreateGraphicsPipelineStateObject(d3d12Desc).Detach();
    mPipelineStateTable.emplace(mPSOInitializers.data() + index, pPSO);
    return pPSO;
}

inline void D3D12PipelineStateManager::InitD3D12PipelineStateDesc(ID3D12RootSignature* pRootSignature, const PipelineInitializer& psoDesc,
    D3D12_GRAPHICS_PIPELINE_STATE_DESC& d3d12Desc)
{
    d3d12Desc.pRootSignature = pRootSignature;
    RHIShader& shader = *psoDesc.mShader;
    const Blob& vsBinary = shader.VertexShader();
    const Blob& hsBinary = shader.HullShader();
    const Blob& dsBinary = shader.DomainShader();
    const Blob& gsBinary = shader.GeometryShader();
    const Blob& psBinary = shader.PixelShader();
    d3d12Desc.VS = { vsBinary.Binary(), vsBinary.Size() };
    d3d12Desc.HS = { hsBinary.Binary(), hsBinary.Size() };
    d3d12Desc.DS = { dsBinary.Binary(), dsBinary.Size() };
    d3d12Desc.GS = { gsBinary.Binary(), gsBinary.Size() };
    d3d12Desc.PS = { psBinary.Binary(), psBinary.Size() };
    const std::vector<ShaderInput>& inputElems = shader.GetInputElements();
    D3D12_INPUT_ELEMENT_DESC* d3dInputElems = inputElems.size() ? new D3D12_INPUT_ELEMENT_DESC[inputElems.size()] : nullptr;
    for (int i = 0; i < inputElems.size(); ++i)
    {
        d3dInputElems[i].Format = ::ConvertToDXGIFormat(inputElems[i].mFormat);
        d3dInputElems[i].SemanticName = inputElems[i].mSemanticName.c_str();
        d3dInputElems[i].SemanticIndex = inputElems[i].mSemanticIndex;
        d3dInputElems[i].InputSlot = inputElems[i].mInputSlot;
        d3dInputElems[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        d3dInputElems[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        d3dInputElems[i].InstanceDataStepRate = 0;
    }
    d3d12Desc.InputLayout = { d3dInputElems, static_cast<uint32_t>(inputElems.size()) };
    d3d12Desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    d3d12Desc.SampleDesc = DXGI_SAMPLE_DESC{
	    static_cast<uint32_t>(psoDesc.mMSAA) >> 4, static_cast<uint32_t>(psoDesc.mMSAA & 0xf)
    };

    d3d12Desc.DSVFormat = ConvertToDXGIFormat(psoDesc.mDepthStencil);
    if (psoDesc.mRenderTarget == Format::UNKNOWN)
    {
	    d3d12Desc.NumRenderTargets = 0;
    }
    else
    {
	    d3d12Desc.RTVFormats[0] = ::ConvertToDXGIFormat(psoDesc.mRenderTarget);
	    d3d12Desc.NumRenderTargets = 1;
    }

    auto& renderTargetBlend = d3d12Desc.BlendState.RenderTarget[0];
    renderTargetBlend.BlendEnable = psoDesc.mOptions & 0b000001;
    if (renderTargetBlend.BlendEnable)
    {
	    renderTargetBlend.RenderTargetWriteMask = static_cast<uint8_t>(psoDesc.mRenderTargetWriteMask);
    	if (psoDesc.mBlendType == BlendType::LOGIC)
    	{
    		renderTargetBlend.LogicOp = ConvertToD3D12LogicOperation(psoDesc.mRenderTargetsBlend.mLogicOp);
    	}
    	else
    	{
    		renderTargetBlend.BlendOp = ConvertToD3D12BlendOperation(psoDesc.mRenderTargetsBlend.mBlendOp);
    		renderTargetBlend.BlendOpAlpha = ConvertToD3D12BlendOperation(psoDesc.mRenderTargetsBlend.mBlendOpAlpha);
    		renderTargetBlend.SrcBlend = ConvertToD3D12Blend(psoDesc.mRenderTargetsBlend.mSrcBlend);
    		renderTargetBlend.DestBlend = ConvertToD3D12Blend(psoDesc.mRenderTargetsBlend.mDestBlend);
    		renderTargetBlend.SrcBlendAlpha = ConvertToD3D12Blend(psoDesc.mRenderTargetsBlend.mSrcBlendAlpha);
    		renderTargetBlend.DestBlendAlpha = ConvertToD3D12Blend(psoDesc.mRenderTargetsBlend.mDestBlendAlpha);
    	}
    }
    else
    {
        renderTargetBlend.RenderTargetWriteMask = static_cast<uint8_t>(psoDesc.mRenderTargetWriteMask);
        if (psoDesc.mBlendType == BlendType::LOGIC)
        {
            renderTargetBlend.LogicOp = D3D12_LOGIC_OP_SET;
        }
        else
        {
            renderTargetBlend = { FALSE,
            FALSE,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE,
            D3D12_BLEND_ZERO,
            D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL};
        }
    }
    d3d12Desc.BlendState.AlphaToCoverageEnable = psoDesc.mBlendOptions;
    d3d12Desc.BlendState.IndependentBlendEnable = false;

    auto& depthTest = d3d12Desc.DepthStencilState;
    depthTest.DepthEnable = psoDesc.mOptions >> 5;
    depthTest.StencilEnable = (psoDesc.mOptions & 0b010000) >> 4;
    depthTest.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    if (depthTest.StencilEnable)
    {
        depthTest.StencilReadMask = psoDesc.mStencilReadMask;
        depthTest.StencilWriteMask = psoDesc.mStencilWriteMask;
        depthTest.BackFace.StencilDepthFailOp = ::ConvertToD3D12StencilOp(psoDesc.mBackStencilDepthFailOp);
        depthTest.BackFace.StencilFailOp = ::ConvertToD3D12StencilOp(psoDesc.mBackStencilFailOp);
        depthTest.BackFace.StencilPassOp = ::ConvertToD3D12StencilOp(psoDesc.mBackStencilPassOp);
        depthTest.BackFace.StencilFunc = ::ConvertToD3D12CompareFunction(psoDesc.mBackStencilFunc);
        depthTest.FrontFace.StencilDepthFailOp = ::ConvertToD3D12StencilOp(psoDesc.mFrontStencilDepthFailOp);
        depthTest.FrontFace.StencilFailOp = ::ConvertToD3D12StencilOp(psoDesc.mFrontStencilFailOp);
        depthTest.FrontFace.StencilPassOp = ::ConvertToD3D12StencilOp(psoDesc.mFrontStencilPassOp);
        depthTest.FrontFace.StencilFunc = ::ConvertToD3D12CompareFunction(psoDesc.mFrontStencilFunc);
    }
    if (depthTest.DepthEnable)
    {
        depthTest.DepthFunc = ::ConvertToD3D12CompareFunction(psoDesc.mDepthCompareFunction);
    }

    d3d12Desc.RasterizerState.CullMode = ::ConvertToD3D12CullMode(psoDesc.mCullMode);
    d3d12Desc.RasterizerState.FillMode = ::ConvertToD3D12DrawMode(psoDesc.mDrawMode);
    d3d12Desc.RasterizerState.ConservativeRaster = psoDesc.mOptions & 0b1000 ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    d3d12Desc.RasterizerState.MultisampleEnable = psoDesc.mOptions & 0b100 > 1;
    d3d12Desc.RasterizerState.AntialiasedLineEnable = psoDesc.mOptions & 0b10000;
    d3d12Desc.RasterizerState.FrontCounterClockwise = psoDesc.mFrontCounterClockWise;
    switch (psoDesc.mDepthBias)
    {
    case DepthBiasSet::NORMAL:
        d3d12Desc.RasterizerState.DepthBias = 0;
        d3d12Desc.RasterizerState.DepthBiasClamp = 0.0f;
        d3d12Desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        d3d12Desc.RasterizerState.DepthClipEnable = true;
        break;
    case DepthBiasSet::TERRAIN:
        d3d12Desc.RasterizerState.DepthBias = 100;
        d3d12Desc.RasterizerState.DepthBiasClamp = 0.0f;
        d3d12Desc.RasterizerState.SlopeScaledDepthBias = 1.0f;
        d3d12Desc.RasterizerState.DepthClipEnable = true;
        break;
    case DepthBiasSet::SHADOW_MAP:
        d3d12Desc.RasterizerState.DepthBias = 2000;
        d3d12Desc.RasterizerState.DepthBiasClamp = 0.0f;
        d3d12Desc.RasterizerState.SlopeScaledDepthBias = 2.0f;
        d3d12Desc.RasterizerState.DepthClipEnable = false;
        break;
    case DepthBiasSet::NONE:
    default:
        d3d12Desc.RasterizerState.DepthBias = 0;
        d3d12Desc.RasterizerState.DepthBiasClamp = 0.0f;
        d3d12Desc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        d3d12Desc.RasterizerState.DepthClipEnable = false;
        break;
    }
    d3d12Desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    d3d12Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    d3d12Desc.NodeMask = 0;
}
#endif
