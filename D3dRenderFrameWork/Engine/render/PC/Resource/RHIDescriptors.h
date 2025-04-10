#pragma once
#include "Engine/pch.h"
#include "Engine/common/Format.h"
class Shader;

struct GraphicPipelineStateDesc
{
    Shader* mShader = nullptr;
    D3D12_PIPELINE_STATE_FLAGS mFlags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ID3D12RootSignature* mRootSignature = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE mPrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    D3D12_BLEND_DESC mBlendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    D3D12_RASTERIZER_DESC mRasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    D3D12_DEPTH_STENCIL_DESC mDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    Format* mRenderTargetFormats = nullptr;
    Format mDepthStencilFormat = Format::D24_UNORM_S8_UINT;
    uint32_t mNumRenderTargetFormats = 0;
    DXGI_SAMPLE_DESC mSampleDesc = {1, 0};
};
