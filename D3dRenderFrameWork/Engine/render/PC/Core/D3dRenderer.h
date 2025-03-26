#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/render/Renderer.h"
#include "Engine/render/PC/Core/D3dCommandList.h"
#include "Engine/render/PC/Resource/RenderTexture.h"

struct RenderList;
class DescriptorHeap;
// class SampleRenderPath;
class Shader;
class RenderPath;
class D3dContext;
class Fence;
class D3dRenderer;

// SampleRenderPath* gSampleRenderPath();

struct GraphicSetting
{
    TextureFormat mBackBufferFormat = TextureFormat::R8G8B8A8_UNORM;
    TextureFormat mDepthStencilFormat = TextureFormat::D24_UNORM_S8_UINT;
    DXGI_SAMPLE_DESC mSampleDesc = {1, 0};
    
    uint64_t mNumPassConstants = 1;
    uint64_t mNumPerObjectConstants = 1;
    uint64_t mMaxRenderItemsPerFrame = 256;
    uint64_t mNumGlobalTexture = 4;
    uint8_t mNumBackBuffers = 3;
    uint16_t mMaxNumRenderTarget = 8;
};

class D3dRenderer : public Renderer
{
    static friend void Renderer::sInitialize();

public:
    static D3dRenderer* sD3dRenderer();
    static uint8_t sCpuWorkingPageIdx();
    static uint8_t sNumCpuWorkPages();
    static const GraphicSetting& sGraphicSettings();
    static bool sIsRenderingThread();
    static std::vector<D3D12_STATIC_SAMPLER_DESC> sGetStaticSamplers();
    static uint64_t sCalcPassCbvStartIdx(const GraphicSetting& graphicSettings, uint8_t cpuWorkingPageIdx);
    static uint64_t sCalcObjectCbvStartIdx(const GraphicSetting& graphicSettings, uint8_t cpuWorkingPageIdx);

    template<typename T>
    void registerRenderPath();
    void registerShaders(const std::vector<Shader*>& shaders);
    ResourceHandle allocateDynamicBuffer(uint64_t size);
    ResourceHandle allocateDefaultBuffer(uint64_t size);
    void updateDynamicBuffer(ResourceHandle buffer, const void* data, uint64_t size);
    void updateDefaultBuffer(ResourceHandle buffer, const void* data, uint64_t size);
    void render();
    void release();
    ~D3dRenderer() override;
    
    DELETE_COPY_OPERATOR(D3dRenderer)
    DELETE_COPY_CONSTRUCTOR(D3dRenderer)
    DELETE_MOVE_OPERATOR(D3dRenderer)
    DELETE_MOVE_CONSTRUCTOR(D3dRenderer)

private:
    void initializeImpl(HWND hWindow);
    void createRootSignature();
    D3dRenderer();

    ID3D12RootSignature* mGlobalRootSignature;

    std::thread::id mRenderingThreadId;

    std::unique_ptr<DescriptorHeap> mRtDescHeap;
    std::unique_ptr<DescriptorHeap> mDsDescHeap;
    std::unique_ptr<DescriptorHeap> mCbSrUaDescHeap;
    
    ComPtr<IDXGISwapChain1> mSwapChain;
    ComPtr<ID3D12CommandQueue> mMainGraphicQueue;
    Fence mFrameFence;
    uint64_t mFrameFenceValue;
    
    RenderTexture2D* mBackBuffers;
    RenderTexture2D mDepthStencilBuffer;
    // FrameResource* mFrameResources;
    uint8_t mCpuWorkingPageIdx;
    
    std::unique_ptr<D3dContext> mD3dContext;
    GraphicSetting mGraphicSettings;
    // std::unordered_map<RenderPath*, RenderData**> mRenderPaths;
    std::unordered_map<Shader*,ID3D12PipelineState*, HashPtrAsTyped<Shader*>> mPipelineStates;

    std::vector<D3dResource*> mResources;
    std::stack<ResourceHandle> mAvailableResourceAddress;

    std::vector<RenderList> mPendingRenderLists;
};

template<typename T, typename = void>
struct CreateRenderPath;

template<typename T>
struct CreateRenderPath<T, std::enable_if_t<std::is_base_of_v<RenderPath, T>>>
{
    T* operator()()
    {
        return new T();
    }
};

template <typename T>
void D3dRenderer::registerRenderPath()
{
    // T* pPath = CreateRenderPath<T>()();
    // D3dAllocator allocator{mD3dContext.get()};
    // RenderData** bufferPtrs = new RenderData*[mGraphicSettings.mNumBackBuffers];
    // pPath->setUp(*mD3dContext);
    // // mRenderPaths.insert(new ForwardRenderPath());
    // for (int i = 0; i < mGraphicSettings.mNumBackBuffers; ++i)
    // {
    //     bufferPtrs[i] = pPath->allocResources(allocator);
    // }
    // mRenderPaths.emplace(pPath, bufferPtrs);
}
#endif
