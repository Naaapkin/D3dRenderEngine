// ReSharper disable CppClangTidyBugproneBranchClone
#pragma once
#include "Engine/pch.h"
#include "Engine/Render/Material.h"
#include "Engine/Render/RenderItem.h"
#include "Engine/Render/RHIDefination.h"
#include "Engine/Render/RHIPipelineStateInializer.h"
#include "Engine/Render/Shader.h"

class RHI;

struct PassConstants
{
    LightConstant mLightConstant;
    FogConstant mFogConstant;
    Float4 mTime; // deltaTime, time, sin(time), cos(time)
};

struct RenderConfiguration
{
    uint8_t mNumBackBuffers = 3;
    bool mEnablePreDepth = false;
    Format mBackBufferFormat = Format::R8G8B8A8_UNORM;
    Format mDepthStencilFormat = Format::D24_UNORM_S8_UINT;
};

class Renderer : public Singleton<Renderer>
{
    friend class Singleton<Renderer>;
public:
    // multi-thread supported
    static std::unique_ptr<Material> createMaterial(ShaderRef shader);

    template<typename TRHI, typename std::enable_if_t<std::is_base_of<RHI, TRHI>::value>>
    TRHI& getRHI() const;

    // single thread only to guarantee the id generation.
    std::unique_ptr<MaterialInstance> createMaterialInstance(const Material& material);

    void initialize();
    void initialize(const RenderConfiguration& configuration);
    void enablePreDepth(std::unique_ptr<RHIShader> preDepthShader);
    void disablePreDepth();
    Float2 getResolution() const;
    void terminate();

    // support multi-thread
	// this func won't check repeated compiling currently, it may cause redundant memory usage.
    std::unique_ptr<RHIShader> compileShader(const Blob& blob, ShaderType shaderTypes, const std::wstring* path = nullptr) const;
    // support multi-thread
    // this func won't check repeated compiling currently, it may cause redundant memory usage.
    std::unique_ptr<RHIShader> compileShader(const Blob& blob, ShaderType shaderTypes, const std::string* path = nullptr) const;
    // support multi-thread
    ShaderRef compileAndRegisterShader(const std::wstring& shaderName, const Blob& blob, ShaderType shaderTypes, const std::wstring* path = nullptr);
    ShaderRef compileAndRegisterShader(const std::string& shaderName, const Blob& blob, ShaderType shaderTypes, const std::string* path = nullptr);
    ShaderRef registerShader(const std::string& name, std::unique_ptr<RHIShader>&& shader);
    // support multi-thread
    void releaseShader(ShaderRef shader);

    void setLightConstants(const LightConstant& lightConstants) const;
    void setTime(float deltaTime, float time) const;
    void setFogConstants(const FogConstant& fogConstants) const;

    VertexBufferRef allocVertexBuffer(uint32_t numVertices, uint32_t vertexSize);
    IndexBufferRef allocIndexBuffer(uint32_t numIndices, Format indexFormat);
    TextureRef allocTexture2D(Format format, uint32_t width, uint32_t height, uint8_t mipLevels);
    void updateVertexBuffer(const void* pData, uint64_t bufferSize, VertexBufferRef vertexBufferGPU, bool blockRendering = true);
    void updateIndexBuffer(const void* pData, uint64_t bufferSize, IndexBufferRef indexBufferGPU, bool blockRendering = true);
    void updateTexture(const void* pData, TextureRef textureGPU, uint8_t mipmap, bool blockRendering = true);
    void appendRenderQueue(NeoRenderQueue* renderQueues, uint32_t numRenderQueues);

#ifdef ENABLE_LEGACY_RENDER_LOOP
    void appendRenderLists(std::unique_ptr<RenderList[]> renderLists, uint32_t numRenderLists);
    void legacyRender();
#endif
    void render();

    Renderer();
    
    NON_COPYABLE(Renderer);
    NON_MOVEABLE(Renderer);
    
private:
    struct RenderContext
    {
        std::unique_ptr<RHIGraphicsContext> mGraphicContext;
        std::unique_ptr<RHIFence> mFenceGPU;
        std::vector<RHIConstantBuffer*> mReleasingCBuffers;
        uint64_t mFenceCPU;
    };
    uint32_t allocGPUResource(RHIObject* pObject);
    //void createBuiltinResources();
    void tryEnablePreDepth();
    void updateMaterialConstants(RenderContext& renderContext, MaterialInstance& materialInstance);
    void neoBeginFrame(RenderContext& renderContext);
	void updateMaterialsInLayer(RenderContext& renderContext, const NeoRenderLayer& layer);
    void neoPreDepthPass(RHIGraphicsContext& graphicsContext, const NeoRenderBatch* renderBatches, uint32_t numBatches);
    void neoSkyboxPass(RHIGraphicsContext& graphicsContext, SkyboxType type, const MaterialInstance* pMaterial);
    void neoOpaquePass(RHIGraphicsContext& graphicsContext, const NeoRenderBatch* renderBatches, uint32_t numBatches);

#ifdef ENABLE_LEGACY_RENDER_LOOP
    void beginFrame(RenderContext& renderContext);
    /*void skyboxPass(RHIGraphicsContext* pRenderContext, const RHIShader& skyboxShader, SkyboxType type);*/
    void depthPrePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderList);
    void opaquePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderBatches);
#endif

    void postRender();

    static constexpr uint8_t PASS_CONSTANT_REGISTER = 0;
    static constexpr uint8_t VIEW_CONSTANT_REGISTER = 1;
    static constexpr uint8_t OBJECT_CONSTANT_REGISTER = 2;

    static ShaderRef sPreDepthShader;
    static IndexBufferRef sQuadMeshIndexBuffer;

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
        PSO_SKY_BOX,
        PSO_SHADOW,
        PSO_OPAQUE,
        PSO_TRANSPARENT,
        NUM_PRESETS
    };
    std::vector<PipelineInitializer> mPipeStateInitializers;
    bool mPreDepthEnabled;

#ifdef ENABLE_LEGACY_RENDER_LOOP
    std::vector<RenderList> mRenderLists;
#endif
    std::vector<NeoRenderQueue> mRenderQueues;

    // ----------------Pass Constants----------------
    std::unique_ptr<PassConstants> mPassConstants;
    // ----------------------------------------------
    
    // -------------GPU Resource Manager-------------
    std::deque<std::unique_ptr<RHIObject>> mGPUResources;
    std::stack<uint64_t>    mAvailableGPUResourceIds;
    // ----------------------------------------------
    
    // ----------------Shader Library----------------
    std::shared_mutex mShaderRegisterMutex;
    std::vector<std::unique_ptr<RHIShader>> mShaders;
    std::stack<uint64_t> mAvailableShaderId;
    // ----------------------------------------------
};

template <typename TRHI, std::enable_if_t<std::is_base_of<RHI, TRHI>::value>>
TRHI& Renderer::getRHI() const
{
	return static_cast<TRHI&>(*mRenderHardwareInterface);
}
