#pragma once
#include "D3dCommandList.h"
#include "D3dContext.h"
#include "RenderContext.h"
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/render/RenderItem.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Graphic.h"
#include "Engine/render/PC/Resource/HlslShader.h"
#include "Engine/render/PC/Resource/RenderTexture.h"
class Shader;

class RenderModule
{
public:
    void setUp(D3D12RHIFactory& allocator)
    {
        
    }

    void loadShaders(std::vector<Shader*>& shaders)
    {
        ShaderCompiler& compiler = ShaderCompiler::getOrInitializeCompiler();
        uint64_t perObjectStart = GraphicSetting::gNumBackBuffers * GraphicSetting::gNumPassConstants;
        uint32_t constantsRegisterCount = GraphicSetting::gNumPassConstants + GraphicSetting::gNumPerObjectConstants;
        std::vector<uint32_t> shaderPropSizes{constantsRegisterCount};
        std::fill_n(shaderPropSizes.begin(), constantsRegisterCount, 256);
        
        for (Shader* shader : shaders)
        {
            ShaderReflector& bufferedReflector = compiler.getReflector(shader->Name());
            std::vector<ShaderProperty>& vsProperties = bufferedReflector.mVsProperties;
            for (ShaderProperty& property : vsProperties)
            {
                if (property.mRegister >= constantsRegisterCount)
                {
                    WARN("shader property bound a index out of preserved register range.")
                    continue;
                }
                if (property.mType == ShaderPropType::CBUFFER)
                {
                    uint64_t newSize = gGetConstantsBufferSize(bufferedReflector.mVsReflector, property.mName);
                    shaderPropSizes[property.mRegister] = std::max<uint64_t>(shaderPropSizes[property.mRegister], newSize);
                }
            }
            std::vector<ShaderProperty>& psProperties = bufferedReflector.mPsProperties;
            for (ShaderProperty& property : psProperties)
            {
                if (property.mRegister >= constantsRegisterCount)
                {
                    WARN("shader property bound a index out of preserved register range.")
                    continue;
                }
                if (property.mType == ShaderPropType::CBUFFER)
                {
                    uint64_t newSize = gGetConstantsBufferSize(bufferedReflector.mPsReflector, property.mName);
                    shaderPropSizes[property.mRegister] = std::max<uint64_t>(shaderPropSizes[property.mRegister], newSize);
                }
            }
        }

        uint64_t numConstantBuffersPerPage = GraphicSetting::gMaxRenderItemsPerFrame * GraphicSetting::gNumPerObjectConstants +
            GraphicSetting::gNumPassConstants * GraphicSetting::gNumBackBuffers;
        D3dContext& d3dContext = D3dContext::instance();
        for (uint32_t i = 0; i < GraphicSetting::gNumBackBuffers; ++i)
        {
            std::vector<DynamicHeap*>& constantBuffers = mRenderData[i].mCBuffers;
            constantBuffers.resize(numConstantBuffersPerPage);
            
            // constants buffer footprint
            uint8_t registerIndex = 0;
            for (uint32_t j = 0; j < GraphicSetting::gNumPassConstants; ++j)
            {
                uint64_t size = shaderPropSizes[registerIndex + j];
                auto& cb = constantBuffers[j];
                cb = new DynamicHeap{mAllocator.allocDynamicBuffer(size)};
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{ cb->gpuAddress(), static_cast<uint32_t>(size) };
                d3dContext.deviceHandle()->CreateConstantBufferView(
                    &cbvDesc, mCbSrUaDescHeap->cpuHandle(GraphicSetting::gNumPassConstants * i + j));
            }
            registerIndex = GraphicSetting::gNumPassConstants;
            for (uint32_t j = 0; j < GraphicSetting::gNumPerObjectConstants; ++j)
            {
                uint64_t size = shaderPropSizes[registerIndex + j];
                for (uint32_t k = 0; k < GraphicSetting::gMaxRenderItemsPerFrame; ++k)
                {
                    auto& cb = constantBuffers[GraphicSetting::gNumPassConstants + GraphicSetting::gNumPerObjectConstants * k + j]; 
                    cb = new DynamicHeap{mAllocator.allocDynamicBuffer(size)};
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{ cb->gpuAddress(), static_cast<uint32_t>(size) };
                    uint64_t offset = perObjectStart + (i * GraphicSetting::gNumPerObjectConstants + k) * GraphicSetting::gNumPerObjectConstants + j;
                    d3dContext.deviceHandle()->CreateConstantBufferView(&cbvDesc, mCbSrUaDescHeap->cpuHandle(offset));
                }
            }
        }
    }
    
    void render(RenderContext& renderContext, D3D12CommandList& d3dCommandList)
    {
    }
    
private:
    static constexpr uint32_t sNumPassConstants = 2;
    
    std::vector<RenderList> mPendingRenderLists;
    
};
