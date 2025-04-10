#include "Engine/common/helper.h"
#ifdef WIN32
#include "Engine/common/Exception.h"
#include "Engine/common/PC/WFunc.h"
#include "Engine/render/PC/D3dUtil.h"

DXGI_FORMAT GetParaInfoFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc) {
    switch (paramDesc.ComponentType)
    {
        case D3D_REGISTER_COMPONENT_UINT32:
            switch (paramDesc.Mask)
            {
                case 1: return DXGI_FORMAT_R32_UINT;   
                case 3: return DXGI_FORMAT_R32G32_UINT;  
                case 7: return DXGI_FORMAT_R32G32B32_UINT;
                case 15: return DXGI_FORMAT_R32G32B32A32_UINT;
                default: break;
            }
            break;
        case D3D_REGISTER_COMPONENT_SINT32:
            switch (paramDesc.Mask)
            {
                case 1: return DXGI_FORMAT_R32_SINT;  
                case 3: return DXGI_FORMAT_R32G32_SINT; 
                case 7: return DXGI_FORMAT_R32G32B32_SINT;
                case 15: return DXGI_FORMAT_R32G32B32A32_SINT;
                default: break;
            }
            break;
        case D3D_REGISTER_COMPONENT_FLOAT32:
            switch (paramDesc.Mask)
            {
                case 1: return DXGI_FORMAT_R32_FLOAT; 
                case 3: return DXGI_FORMAT_R32G32_FLOAT;
                case 7: return DXGI_FORMAT_R32G32B32_FLOAT;
                case 15: return DXGI_FORMAT_R32G32B32A32_FLOAT;
                default: break;
            }
            break;
        default: break;
    }
    return DXGI_FORMAT_UNKNOWN;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPipelineStateDesc()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;	// counter-clock wise
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
    return psoDesc;
}

uint64_t gGetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::string& name)
{
    D3D12_SHADER_BUFFER_DESC bufferDesc{};
    auto* pBuffer = pReflector->GetConstantBufferByName(name.c_str());
    pBuffer->GetDesc(&bufferDesc);
    return bufferDesc.Size;
}

uint64_t gGetConstantsBufferSize(ID3D12ShaderReflection* pReflector, const std::wstring& name)
{
    return gGetConstantsBufferSize(pReflector, ::Utf8ToAscii(name));
}

bool gImplicitTransit(const uint32_t stateBefore, uint32_t& stateAfter,
                      bool isBufferOrSimultaneous)
{
    constexpr uint32_t READ_ONLY_MASK = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER |
        D3D12_RESOURCE_STATE_INDEX_BUFFER |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT | 
        D3D12_RESOURCE_STATE_COPY_SOURCE |
        D3D12_RESOURCE_STATE_DEPTH_READ;
    constexpr uint32_t WRITE_ONLY_MASK = D3D12_RESOURCE_STATE_COPY_DEST | 
        D3D12_RESOURCE_STATE_RENDER_TARGET |
        D3D12_RESOURCE_STATE_STREAM_OUT;
	constexpr uint32_t DEPTH_RW = D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ;
	constexpr uint32_t SHADER_COPY = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | 
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | 
        D3D12_RESOURCE_STATE_COPY_DEST | 
        D3D12_RESOURCE_STATE_COPY_SOURCE;

    // first: check if this is a read to write transition
    uint32_t writeStateBefore = stateBefore & WRITE_ONLY_MASK;
    uint32_t writeStateAfter = stateAfter & WRITE_ONLY_MASK;

#if defined(DEBUG) or defined(_DEBUG)
    ASSERT(((writeStateAfter - 1) & writeStateAfter && writeStateAfter) == 0, TEXT("cant transition to multi-write state"));
#endif
    if (((stateBefore | stateAfter) & DEPTH_RW) ||                          // depth read or write states cant implicitly transition in or out
		(stateBefore && (writeStateAfter ^ writeStateBefore)) ||            // for any transition from a promoted state to another state that has different write state, a explicit transition is needed.
		(!isBufferOrSimultaneous && (stateAfter & ~SHADER_COPY))) {         // if not buffer or simultaneous, cant transition to states except shader rw and copy src/dst
        return false;
    }

    stateAfter |= stateBefore;
    return true;

}
#endif
