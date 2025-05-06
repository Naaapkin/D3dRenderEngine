#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/RHIDescriptors.h"
#include "Engine/common/Exception.h"

static CommandListType ConvertFromD3D12CommandListType(D3D12_COMMAND_LIST_TYPE type);
static Format ConvertFromDXGIFormat(DXGI_FORMAT format);
static DXGI_FORMAT ConvertToDXGIFormat(Format format);
static D3D12_COMPARISON_FUNC ConvertToD3D12CompareFunction(CompareFunction function);
static D3D12_BLEND_OP ConvertToD3D12BlendOperation(BlendOperation operation);
static D3D12_LOGIC_OP ConvertToD3D12LogicOperation(LogicOperation operation);
static D3D12_BLEND ConvertToD3D12Blend(BlendMode blend);
static D3D12_STENCIL_OP ConvertToD3D12StencilOp(StencilOperation op);
static D3D12_CULL_MODE ConvertToD3D12CullMode(CullMode mode);
static D3D12_FILL_MODE ConvertToD3D12DrawMode(DrawMode mode);
static D3D12_COMMAND_LIST_TYPE ConvertToD3D12CommandListType(CommandListType type);

inline CommandListType ConvertFromD3D12CommandListType(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return CommandListType::GRAPHIC;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return CommandListType::COMPUTE;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        return CommandListType::COPY;
    default:
        THROW_EXCEPTION(TEXT("Unknown D3D12 command list type"));
    }
}

Format ConvertFromDXGIFormat(DXGI_FORMAT format)
{
    switch (format) {
        case DXGI_FORMAT_R8_UNORM:          return Format::R8_UNORM;
        case DXGI_FORMAT_R8G8_UNORM:        return Format::R8G8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM:    return Format::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return Format::R8G8B8A8_UNORM_SRGB;
        case DXGI_FORMAT_R8_SNORM:          return Format::R8_SNORM;
        case DXGI_FORMAT_R8G8_SNORM:        return Format::R8G8_SNORM;
        case DXGI_FORMAT_R8G8B8A8_SNORM:    return Format::R8G8B8A8_SNORM;
        case DXGI_FORMAT_R8_UINT:           return Format::R8_UINT;
        case DXGI_FORMAT_R8G8_UINT:         return Format::R8G8_UINT;
        case DXGI_FORMAT_R8G8B8A8_UINT:     return Format::R8G8B8A8_UINT;
        case DXGI_FORMAT_R8_SINT:           return Format::R8_SINT;
        case DXGI_FORMAT_R8G8_SINT:         return Format::R8G8_SINT;
        case DXGI_FORMAT_R8G8B8A8_SINT:     return Format::R8G8B8A8_SINT;
        case DXGI_FORMAT_R16_UNORM:         return Format::R16_UNORM;
        case DXGI_FORMAT_R16G16_UNORM:      return Format::R16G16_UNORM;
        case DXGI_FORMAT_R16G16B16A16_UNORM: return Format::R16G16B16A16_UNORM;
        case DXGI_FORMAT_R16_SNORM:         return Format::R16_SNORM;
        case DXGI_FORMAT_R16G16_SNORM:      return Format::R16G16_SNORM;
        case DXGI_FORMAT_R16G16B16A16_SNORM: return Format::R16G16B16A16_SNORM;
        case DXGI_FORMAT_R16_UINT:          return Format::R16_UINT;
        case DXGI_FORMAT_R16G16_UINT:       return Format::R16G16_UINT;
        case DXGI_FORMAT_R16G16B16A16_UINT: return Format::R16G16B16A16_UINT;
        case DXGI_FORMAT_R16_SINT:          return Format::R16_SINT;
        case DXGI_FORMAT_R16G16_SINT:       return Format::R16G16_SINT;
        case DXGI_FORMAT_R16G16B16A16_SINT: return Format::R16G16B16A16_SINT;
        case DXGI_FORMAT_R32_UINT:          return Format::R32_UINT;
        case DXGI_FORMAT_R32G32_UINT:       return Format::R32G32_UINT;
        case DXGI_FORMAT_R32G32B32_UINT:    return Format::R32G32B32_UINT;
        case DXGI_FORMAT_R32G32B32A32_UINT: return Format::R32G32B32A32_UINT;
        case DXGI_FORMAT_R32_SINT:          return Format::R32_SINT;
        case DXGI_FORMAT_R32G32_SINT:       return Format::R32G32_SINT;
        case DXGI_FORMAT_R32G32B32_SINT:    return Format::R32G32B32_SINT;
        case DXGI_FORMAT_R32G32B32A32_SINT: return Format::R32G32B32A32_SINT;
        case DXGI_FORMAT_R32_TYPELESS:      return Format::R32_TYPELESS;
        case DXGI_FORMAT_R32G32_TYPELESS:   return Format::R32G32_TYPELESS;
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return Format::R32G32B32A32_TYPELESS;
        case DXGI_FORMAT_R32_FLOAT:         return Format::R32_FLOAT;
        case DXGI_FORMAT_R32G32_FLOAT:      return Format::R32G32_FLOAT;
        case DXGI_FORMAT_R32G32B32_FLOAT:      return Format::R32G32B32_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_FLOAT: return Format::R32G32B32A32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT: return Format::D24_UNORM_S8_UINT;

        default:
            THROW_EXCEPTION(TEXT("Unsupported DXGI_FORMAT"));
            return Format::UNKNOWN;
    }
}

DXGI_FORMAT ConvertToDXGIFormat(Format format)
{
    switch (format) {
        case Format::R8_UNORM:          return DXGI_FORMAT_R8_UNORM;
        case Format::R8G8_UNORM:        return DXGI_FORMAT_R8G8_UNORM;
        case Format::R8G8B8A8_UNORM:    return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case Format::R8_SNORM:          return DXGI_FORMAT_R8_SNORM;
        case Format::R8G8_SNORM:        return DXGI_FORMAT_R8G8_SNORM;
        case Format::R8G8B8A8_SNORM:    return DXGI_FORMAT_R8G8B8A8_SNORM;
        case Format::R8_UINT:           return DXGI_FORMAT_R8_UINT;
        case Format::R8G8_UINT:         return DXGI_FORMAT_R8G8_UINT;
        case Format::R8G8B8A8_UINT:     return DXGI_FORMAT_R8G8B8A8_UINT;
        case Format::R8_SINT:           return DXGI_FORMAT_R8_SINT;
        case Format::R8G8_SINT:         return DXGI_FORMAT_R8G8_SINT;
        case Format::R8G8B8A8_SINT:     return DXGI_FORMAT_R8G8B8A8_SINT;
        case Format::R16_UNORM:         return DXGI_FORMAT_R16_UNORM;
        case Format::R16G16_UNORM:      return DXGI_FORMAT_R16G16_UNORM;
        case Format::R16G16B16A16_UNORM: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case Format::R16_SNORM:         return DXGI_FORMAT_R16_SNORM;
        case Format::R16G16_SNORM:      return DXGI_FORMAT_R16G16_SNORM;
        case Format::R16G16B16A16_SNORM: return DXGI_FORMAT_R16G16B16A16_SNORM;
        case Format::R16_UINT:          return DXGI_FORMAT_R16_UINT;
        case Format::R16G16_UINT:       return DXGI_FORMAT_R16G16_UINT;
        case Format::R16G16B16A16_UINT: return DXGI_FORMAT_R16G16B16A16_UINT;
        case Format::R16_SINT:          return DXGI_FORMAT_R16_SINT;
        case Format::R16G16_SINT:       return DXGI_FORMAT_R16G16_SINT;
        case Format::R16G16B16A16_SINT: return DXGI_FORMAT_R16G16B16A16_SINT;
        case Format::R32_TYPELESS:      return DXGI_FORMAT_R32_TYPELESS;
        case Format::R32G32_TYPELESS:   return DXGI_FORMAT_R32G32_TYPELESS;
        case Format::R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case Format::R32_FLOAT:         return DXGI_FORMAT_R32_FLOAT;
        case Format::R32G32_FLOAT:      return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32G32B32_FLOAT:   return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32B32A32_FLOAT: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32_UINT:          return DXGI_FORMAT_R32_UINT;
        case Format::R32G32_UINT:       return DXGI_FORMAT_R32G32_UINT;
        case Format::R32G32B32_UINT:    return DXGI_FORMAT_R32G32B32_UINT;
        case Format::R32G32B32A32_UINT: return DXGI_FORMAT_R32G32B32A32_UINT;
        case Format::R32_SINT:          return DXGI_FORMAT_R32_SINT;
        case Format::R32G32_SINT:       return DXGI_FORMAT_R32G32_SINT;
        case Format::R32G32B32_SINT:    return DXGI_FORMAT_R32G32B32_SINT;
        case Format::R32G32B32A32_SINT: return DXGI_FORMAT_R32G32B32A32_SINT;
        case Format::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;

        case Format::UNKNOWN:
        default:
            THROW_EXCEPTION(TEXT("Unsupported Format"));
            return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_COMPARISON_FUNC ConvertToD3D12CompareFunction(CompareFunction function)
{
    switch (function) {
    case CompareFunction::ALWAYS:        return D3D12_COMPARISON_FUNC_ALWAYS;
    case CompareFunction::NEVER:         return D3D12_COMPARISON_FUNC_NEVER;
    case CompareFunction::LESS:          return D3D12_COMPARISON_FUNC_LESS;
    case CompareFunction::EQUAL:         return D3D12_COMPARISON_FUNC_EQUAL;
    case CompareFunction::LESS_EQUAL:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case CompareFunction::GREATER:       return D3D12_COMPARISON_FUNC_GREATER;
    case CompareFunction::NOT_EQUAL:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case CompareFunction::GREATER_EQUAL: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    default:
        THROW_EXCEPTION(TEXT("Unsupported CompareFunction"));
    }
}

D3D12_BLEND_OP ConvertToD3D12BlendOperation(BlendOperation operation)
{
    switch (operation)
    {
    case BlendOperation::ADD:
        return D3D12_BLEND_OP_ADD;
    case BlendOperation::SUBTRACT:
        return D3D12_BLEND_OP_SUBTRACT;
    case BlendOperation::REV_SUBTRACT:
        return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOperation::MIN:
        return D3D12_BLEND_OP_MIN;
    case BlendOperation::MAX:
        return D3D12_BLEND_OP_MAX;
    default:
        THROW_EXCEPTION(TEXT("Unsupported BlendOperation"));
    }
}

D3D12_LOGIC_OP ConvertToD3D12LogicOperation(LogicOperation operation)
{
    switch (operation) {
    case LogicOperation::CLEAR:         return D3D12_LOGIC_OP_CLEAR;
    case LogicOperation::SET:          return D3D12_LOGIC_OP_SET;
    case LogicOperation::COPY:         return D3D12_LOGIC_OP_COPY;
    case LogicOperation::COPY_INVERTED: return D3D12_LOGIC_OP_COPY_INVERTED;
    case LogicOperation::NOOP:         return D3D12_LOGIC_OP_NOOP;
    case LogicOperation::INVERT:       return D3D12_LOGIC_OP_INVERT;
    case LogicOperation::AND:          return D3D12_LOGIC_OP_AND;
    case LogicOperation::NAND:         return D3D12_LOGIC_OP_NAND;
    case LogicOperation::OR:           return D3D12_LOGIC_OP_OR;
    case LogicOperation::NOR:          return D3D12_LOGIC_OP_NOR;
    case LogicOperation::XOR:          return D3D12_LOGIC_OP_XOR;
    case LogicOperation::EQUIV:        return D3D12_LOGIC_OP_EQUIV;
    case LogicOperation::AND_REVERSE:  return D3D12_LOGIC_OP_AND_REVERSE;
    case LogicOperation::AND_INVERTED: return D3D12_LOGIC_OP_AND_INVERTED;
    case LogicOperation::OR_REVERSE:   return D3D12_LOGIC_OP_OR_REVERSE;
    case LogicOperation::OR_INVERTED:  return D3D12_LOGIC_OP_OR_INVERTED;

    default:
        THROW_EXCEPTION(TEXT("unsupported logic operation."));
    }
}

D3D12_BLEND ConvertToD3D12Blend(BlendMode blend)
{
    switch (blend) {
    case BlendMode::ZERO:               return D3D12_BLEND_ZERO;
    case BlendMode::ONE:                return D3D12_BLEND_ONE;
    case BlendMode::SRC_COLOR:          return D3D12_BLEND_SRC_COLOR;
    case BlendMode::INV_SRC_COLOR:      return D3D12_BLEND_INV_SRC_COLOR;
    case BlendMode::SRC_ALPHA:          return D3D12_BLEND_SRC_ALPHA;
    case BlendMode::INV_SRC_ALPHA:      return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendMode::DEST_ALPHA:         return D3D12_BLEND_DEST_ALPHA;
    case BlendMode::INV_DEST_ALPHA:     return D3D12_BLEND_INV_DEST_ALPHA;
    case BlendMode::DEST_COLOR:         return D3D12_BLEND_DEST_COLOR;
    case BlendMode::INV_DEST_COLOR:     return D3D12_BLEND_INV_DEST_COLOR;
    case BlendMode::SRC_ALPHA_SAT:      return D3D12_BLEND_SRC_ALPHA_SAT;
    case BlendMode::BLEND_FACTOR:       return D3D12_BLEND_BLEND_FACTOR;
    case BlendMode::INV_BLEND_FACTOR:   return D3D12_BLEND_INV_BLEND_FACTOR;
    case BlendMode::SRC1_COLOR:         return D3D12_BLEND_SRC1_COLOR;
    case BlendMode::INV_SRC1_COLOR:     return D3D12_BLEND_INV_SRC1_COLOR;
    case BlendMode::SRC1_ALPHA:         return D3D12_BLEND_SRC1_ALPHA;
    case BlendMode::INV_SRC1_ALPHA:     return D3D12_BLEND_INV_SRC1_ALPHA;
    case BlendMode::ALPHA_FACTOR:       return D3D12_BLEND_BLEND_FACTOR;      // 注意：D3D12没有单独的ALPHA_FACTOR
    case BlendMode::INV_ALPHA_FACTOR:   return D3D12_BLEND_INV_BLEND_FACTOR;  // 注意：D3D12没有单独的INV_ALPHA_FACTOR

    default:
        THROW_EXCEPTION(TEXT("unsupported blend mode."));
    }
}

D3D12_STENCIL_OP ConvertToD3D12StencilOp(StencilOperation op)
{
    switch (op)
    {
    case StencilOperation::KEEP:      return D3D12_STENCIL_OP_KEEP;
    case StencilOperation::ZERO:      return D3D12_STENCIL_OP_ZERO;
    case StencilOperation::REPLACE:   return D3D12_STENCIL_OP_REPLACE;
    case StencilOperation::INCR_SAT:  return D3D12_STENCIL_OP_INCR_SAT;
    case StencilOperation::DECR_SAT:  return D3D12_STENCIL_OP_DECR_SAT;
    case StencilOperation::INVERT:    return D3D12_STENCIL_OP_INVERT;
    case StencilOperation::INCR:      return D3D12_STENCIL_OP_INCR;
    case StencilOperation::DECR:      return D3D12_STENCIL_OP_DECR;
    default:                          return D3D12_STENCIL_OP_KEEP; // Fallback
    }
}

D3D12_CULL_MODE ConvertToD3D12CullMode(CullMode mode)
{
    switch (mode)
    {
    case CullMode::BACK:
        return D3D12_CULL_MODE_BACK;
    case CullMode::FRONT:
        return D3D12_CULL_MODE_FRONT;
    case CullMode::NONE:
        return D3D12_CULL_MODE_NONE;
    default:
        THROW_EXCEPTION(TEXT("unsupported cull mode."));
    }
}

D3D12_FILL_MODE ConvertToD3D12DrawMode(DrawMode mode)
{
    switch (mode)
    {
    case DrawMode::SOLID:
        return D3D12_FILL_MODE_SOLID;
    case DrawMode::WIREFRAME:
        return D3D12_FILL_MODE_WIREFRAME;
    default:
        THROW_EXCEPTION(TEXT("unsupported draw mode."));
    }
}

D3D12_COMMAND_LIST_TYPE ConvertToD3D12CommandListType(CommandListType type)
{
    switch (type)
    {
    case CommandListType::GRAPHIC:
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case CommandListType::COPY:
        return D3D12_COMMAND_LIST_TYPE_COPY;
    case CommandListType::COMPUTE:
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    default:
        THROW_EXCEPTION(TEXT("unsupported command list type."));
    }
}

inline D3D12_SRV_DIMENSION ConvertToD3D12SRVDimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::TEXTURE1D:
        return D3D12_SRV_DIMENSION_TEXTURE1D;
    case TextureDimension::TEXTURE2D:
        return D3D12_SRV_DIMENSION_TEXTURE2D;
    case TextureDimension::TEXTURE3D:
        return D3D12_SRV_DIMENSION_TEXTURE3D;
    case TextureDimension::TEXTURE_CUBE:
        return D3D12_SRV_DIMENSION_TEXTURECUBE;
    case TextureDimension::TEXTURE1D_ARRAY:
        return D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    case TextureDimension::TEXTURE2D_ARRAY:
        return D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    default:
        WARN("unsupported texture dimension.");
        return D3D12_SRV_DIMENSION_UNKNOWN;
    }
}

inline TextureDimension ConvertFromD3DSRVFormat(D3D_SRV_DIMENSION dimension)
{
    switch (dimension)
    {
    case D3D_SRV_DIMENSION_BUFFER:
        return TextureDimension::BUFFER;
    case D3D_SRV_DIMENSION_TEXTURE1D:
        return TextureDimension::TEXTURE1D;
    case D3D_SRV_DIMENSION_TEXTURE2D:
        return TextureDimension::TEXTURE2D;
    case D3D_SRV_DIMENSION_TEXTURE3D:
        return TextureDimension::TEXTURE3D;
    case D3D_SRV_DIMENSION_TEXTURECUBE:
        return TextureDimension::TEXTURE_CUBE;
    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
        return TextureDimension::TEXTURE1D_ARRAY;
    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
        return TextureDimension::TEXTURE2D_ARRAY;
    default:
        WARN("Unsupported D3D SRV dimension.");
        return TextureDimension::TEXTURE2D; // 或者定义一个 UNKNOWN 值
    }
}

inline D3D12_SHADER_RESOURCE_VIEW_DESC ConvertToD3D12SRVDesc(RHITextureDesc desc)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC d3d12Desc;
    d3d12Desc.Format = ::ConvertToDXGIFormat(desc.mFormat);
    d3d12Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch (desc.mDimension)
    {
    case TextureDimension::TEXTURE1D:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
        d3d12Desc.Texture1D.MipLevels = desc.mMipLevels;
        d3d12Desc.Texture1D.MostDetailedMip = 0;
        return d3d12Desc;
    case TextureDimension::TEXTURE2D:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        d3d12Desc.Texture2D.MipLevels = desc.mMipLevels;
        d3d12Desc.Texture2D.PlaneSlice = 0;
        d3d12Desc.Texture2D.MostDetailedMip = 0;
        return d3d12Desc;
    case TextureDimension::TEXTURE3D:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        d3d12Desc.Texture3D.MipLevels = desc.mMipLevels;
        d3d12Desc.Texture3D.MostDetailedMip = 0;
        return d3d12Desc;
    case TextureDimension::TEXTURE_CUBE:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        d3d12Desc.TextureCube.MipLevels = desc.mMipLevels;
        d3d12Desc.TextureCube.MostDetailedMip = 0;
        return d3d12Desc;
    case TextureDimension::TEXTURE1D_ARRAY:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        d3d12Desc.Texture1DArray.MipLevels = desc.mMipLevels;
        d3d12Desc.Texture1DArray.MostDetailedMip = 0;
        d3d12Desc.Texture1DArray.ArraySize = desc.mDepth;
        d3d12Desc.Texture1DArray.FirstArraySlice = 0;
        return d3d12Desc;
    case TextureDimension::TEXTURE2D_ARRAY:
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
        d3d12Desc.Texture2DArray.MipLevels = desc.mMipLevels;
        d3d12Desc.Texture2DArray.MostDetailedMip = 0;
        d3d12Desc.Texture2DArray.ArraySize = desc.mDepth;
        d3d12Desc.Texture2DArray.FirstArraySlice = 0;
        return d3d12Desc;
    default:
        WARN("unsupported texture dimension.");
        d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
        return d3d12Desc;
    }
}
#endif
