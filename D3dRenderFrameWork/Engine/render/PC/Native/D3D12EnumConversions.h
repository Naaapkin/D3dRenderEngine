#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/RHIDescriptors.h"
#include "Engine/common/Exception.h"

static Format ConvertFromDXGIFormat(DXGI_FORMAT format);
static DXGI_FORMAT ConvertToDXGIFormat(Format format);
static D3D12_COMPARISON_FUNC ConvertToD3D12CompareFunction(CompareFunction function);
static D3D12_BLEND_OP ConvertToD3D12BlendOperation(BlendOperation operation);
static D3D12_LOGIC_OP ConvertToD3D12LogicOperation(LogicOperation operation);
static D3D12_BLEND ConvertToD3D12Blend(BlendMode blend);
static D3D12_CULL_MODE ConvertToD3D12CullMode(CullMode mode);
static D3D12_FILL_MODE ConvertToD3D12DrawMode(DrawMode mode);
static D3D12_COMMAND_LIST_TYPE ConvertToD3D12CommandListType(CommandListType type);

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
    case D3D12_COMPARISON_FUNC_ALWAYS:   return D3D12_COMPARISON_FUNC_NEVER;
    case CompareFunction::NEVER:          return D3D12_COMPARISON_FUNC_NEVER;
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
        case D3D12_BLEND_OP_ADD:
            return D3D12_BLEND_OP_ADD;
        case D3D12_BLEND_OP_SUBTRACT:
            return D3D12_BLEND_OP_SUBTRACT;
        case D3D12_BLEND_OP_REV_SUBTRACT:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case D3D12_BLEND_OP_MIN:
            return D3D12_BLEND_OP_MIN;
        case D3D12_BLEND_OP_MAX:
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
#endif
