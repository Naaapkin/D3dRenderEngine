#ifdef WIN32
#include <Engine/common/PC/WException.h>
#include <Engine/common/PC/WFunc.h>
#include <Engine/render/PC/D3dUtil.h>

DXGI_FORMAT GetParaFormatFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc) {
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

ID3DBlob* LoadCompiledShaderObject(const String& path)
{
    std::ifstream fIn{ path, std::ios::binary };
    if (!fIn.is_open()){
        //char buffer[256];
        //_getcwd(buffer, 256);
        return nullptr;
    }
    fIn.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = fIn.tellg();
    fIn.seekg(0, std::ios_base::beg);
    ID3DBlob* binaryBlob;
    ThrowIfFailed(D3DCreateBlob(size, &binaryBlob));
    fIn.read(static_cast<char*>(binaryBlob->GetBufferPointer()), size);
    fIn.close();
    return binaryBlob;
}

bool CanImplicitTransit(const uint32_t& stateBefore, uint32_t& stateAfter,
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

#ifdef DEBUG || _ DEBUG
    ASSERT(((writeStateAfter - 1) & writeStateAfter && writeStateAfter) == 0, TEXT("cant transition to multi-write state"));
#endif
    if (((stateBefore | stateAfter) & DEPTH_RW) ||                          // depth read or write states cant implicit transition in or out
		(writeStateBefore != 0 && (writeStateAfter ^ writeStateBefore)) ||  // write state change including no write to write, write to no write, write to another write
		(!isBufferOrSimultaneous || (stateAfter & ~SHADER_COPY))) {         // if not buffer or simultaneous, cant transition to states except shader rw and copy src/dst
        return false;
    }

    stateAfter |= stateBefore;
    return true;

}
#endif
