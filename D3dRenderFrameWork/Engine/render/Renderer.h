// ReSharper disable CppClangTidyBugproneBranchClone
#pragma once
#include "Material.h"
#include "RenderItem.h"
#include "RHIDefination.h"
#include "RHIPipelineStateInializer.h"
#include "Shader.h"
#include "PC/D3D12RHI.h"
#include "Engine/pch.h"

class RenderTargetHandle
{
public:
    RenderTargetRef GetRenderTargetRef() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Format GetFormat() const;
    bool MSAAEnabled() const;
    RenderTargetHandle();
    RenderTargetHandle(RenderTargetRef renderTargetRef, const RHITextureDesc& desc);;
    
private:
    RenderTargetRef mRTRef;
    RHITextureDesc mDesc;
};

class DepthStencilHandle
{
public:
    DepthStencilRef GetRenderTargetRef() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Format GetFormat() const;
    bool MSAAEnabled() const;
    bool MipmapEnabled() const;
    DepthStencilHandle();
    DepthStencilHandle(RenderTargetRef depthStencilRef, const RHITextureDesc& desc);;
    
private:
    DepthStencilRef mDSRef;
    RHITextureDesc mDesc;   // cache, for fast validation.
};

class Renderer : public Singleton<Renderer>
{
    friend class Singleton<Renderer>;
public:
    // multi-thread supported
    static std::unique_ptr<Material> createMaterial(RHIShader* pShader);

    // single thread only to guarantee the id generation.
    static std::unique_ptr<MaterialInstance> createMaterialInstance(const Material& material);

    void initialize();

    // support multi-thread
	// this func won't check repeated compiling currently, it may cause redundant memory usage.
	// TODO: avoid repeat compiling.
    std::unique_ptr<RHIShader> compileShader(const Blob& blob, ShaderType shaderTypes, const String* path = nullptr) const;

    RHIShader* registerShader(const String& name, std::unique_ptr<RHIShader>&& shader);

    void appendRenderLists(std::unique_ptr<RenderList[]> renderLists, uint32_t numRenderLists)
    {
        mRenderLists.reserve(mRenderLists.size() + numRenderLists);
        mRenderLists.insert(mRenderLists.end(), renderLists.get(), renderLists.get() + numRenderLists);
    }

    VertexBufferRef allocVertexBuffer(uint32_t numVertices, uint32_t vertexSize);
    IndexBufferRef allocIndexBuffer(uint32_t numIndices, Format indexFormat);
    TextureRef allocTexture2D(Format format, uint32_t width, uint32_t height, uint8_t mipLevels);
    void updateVertexBuffer(const void* pData, uint64_t bufferSize, VertexBufferRef vertexBufferGPU, bool blockRendering = true);
    void updateIndexBuffer(const void* pData, uint64_t bufferSize, IndexBufferRef indexBufferGPU, bool blockRendering = true);
    void updateTexture(const void* pData, TextureRef textureGPU, bool blockRendering = true);

    //void releaseMaterialInstance(uint64_t instanceId);
    void render();

    Renderer();
    
    NON_COPYABLE(Renderer);
    NON_MOVEABLE(Renderer);
    
private:
    void createBuiltinResources();
    void beginFrame(RHIGraphicsContext* renderContext);
    void depthPrePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants);
    void opaquePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants);
    void postRender();

    struct RenderContext
    {
        std::unique_ptr<RHIGraphicsContext> mGraphicContext;
        std::unique_ptr<RHIFence> mFenceGPU;
        uint64_t mFenceCPU;
    };

    RHI* mRenderHardwareInterface;
    
    std::unique_ptr<RHISwapChain> mSwapChain;
    RHIDepthStencil* mDepthStencilBuffer;
    std::unique_ptr<RenderContext[]> mRenderContexts;
    uint8_t mNumRenderContexts;
    uint8_t mCurrentRenderContextIndex;

    std::unique_ptr<RHICopyContext> mCopyContext;
    std::unique_ptr<RHIFence> mCopyFenceGPU;
    uint64_t mCopyFenceCPU;

    enum PsoPresets : uint16_t
    {
        PSO_PRE_DEPTH,
        PSO_SHADOW,
        PSO_OPAQUE,
        PSO_TRANSPARENT,
        NUM_PRESETS
    };
    std::vector<PipelineInitializer> mPipeStateInitializers;

    std::vector<RenderList> mRenderLists;
    
    // -------------GPU Resource Manager-------------
    std::vector<std::unique_ptr<RHIObject>> mGPUResources;
    std::stack<uint64_t>    mAvailableGPUResourceIds;
    // ----------------------------------------------
    
    // ----------------Shader Library----------------
    std::shared_mutex mShaderRegisterMutex;
    std::vector<std::unique_ptr<RHIShader>> mShaders;
    std::stack<uint64_t> mAvailableShaderId;
    
    std::unordered_map<String, RHIShader*> mShaderMap;
    // ----------------------------------------------

    
    // ---------------Material Engine----------------
    /*std::vector<MaterialInstance> mMaterialInstances;
    std::stack<uint64_t> mAvailableMaterialInstanceId;*/
    // ----------------------------------------------
};
