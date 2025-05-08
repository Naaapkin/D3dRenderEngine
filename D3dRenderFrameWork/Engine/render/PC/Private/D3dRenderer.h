#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/render/Renderer.h"
#include "Engine/render/PC/Private/DescriptorHeap.h"

// SampleRenderPath* gSampleRenderPath();

struct ConstantProperty;
struct RenderList;
class HlslShader;
class D3dContext;

struct RenderData
{
    std::vector<DynamicHeap*> mCBuffers;
};

class D3dRenderer : public Renderer
{
    static friend void Renderer::sInitialize();

public:
    static D3dRenderer* sD3dRenderer();
    static uint8_t sCpuWorkingPageIdx();
    static bool sIsRenderingThread();
    static uint64_t sCalcPassCbvStartIdx(uint8_t cpuWorkingPageIdx);
    static uint64_t sCalcObjectCbvOffset(uint8_t cpuWorkingPageIdx);

    void initialize(RendererSetting setting) override;
    void registerShaders(std::vector<RHISubShader*> shaders) override;
    // ResourceHandle allocateDynamicBuffer(uint64_t Size) override;
    void releaseResource(const ResourceHandle& resourceHandle) const override;
    // void updatePassConstants(uint8_t registerIndex, void* pData, uint64_t Size) override;
    // void appendRenderLists(std::vector<RenderList>&& renderLists) override;
    void updateDynamicResource(const ResourceHandle& resourceHandle, const void* data) const override;
    void updateStaticResource(const ResourceHandle& resourceHandle, const void* data) const override;   // TODO: use lambda
    void render() override;
    void release() override;
    ~D3dRenderer() override;

    // ---------------temp---------------
    // ----------------------------------
    
    DELETE_COPY_OPERATOR(D3dRenderer)
    DELETE_COPY_CONSTRUCTOR(D3dRenderer)
    DELETE_MOVE_OPERATOR(D3dRenderer)
    DELETE_MOVE_CONSTRUCTOR(D3dRenderer)

private:
    void onPreRender();
    void onRender();
    void createRootSignature();
    D3dRenderer();

    std::thread::id mRenderingThreadId;

    std::unique_ptr<D3D12FrameResource[]> mFrameResources;

    // ----------------------Renderer Settings------------------------
    Format mBackBufferFormat = Format::R32G32B32A32_FLOAT;
    Format mDepthStencilFormat = Format::D24_UNORM_S8_UINT;
    uint8_t mNumBackBuffers = 3;
    uint8_t mMsaaSampleCount = 2;
    uint8_t mMsaaSampleQuality = 1;
    bool mEnableMsaa = false;
    // ---------------------------------------------------------------

    ComPtr<ID3D12RootSignature> mGlobalRootSignature;
    std::unordered_map<const RHISubShader*, ID3D12PipelineState*, HashPtrAsTyped<const RHISubShader*>> mPipelineStates;
    std::unique_ptr<DescriptorHeap> mRtDescHeap;
    std::unique_ptr<DescriptorHeap> mDsDescHeap;
    std::unique_ptr<DescriptorHeap> mCbSrUaDescHeap;

    ComPtr<IDXGISwapChain1> mSwapChain;
    StaticHeap* mBackBuffers;           // TODO: delete ptr
    StaticHeap mDepthStencilBuffer;
    uint8_t mCpuWorkingPageIdx;

    // -----------------------Resources Pool--------------------------
    uint8_t mNumDirtyFrames;
    std::vector<DynamicHeap>* mCBuffers;    //TODO: delete ptr
    std::vector<D3dResource*> mResources;
    std::stack<uint64_t> mAvailableResourceAddresses;
    std::vector<uint64_t>* mReleasingResources;
    // ---------------------------------------------------------------

    // ---------------------Command Queues----------------------------
    ComPtr<ID3D12CommandQueue> mMainComputeQueue;
    
    RenderContext mCopyContext;
    ComPtr<ID3D12CommandQueue> mCopyQueue;
    Fence mCopyFence;
    uint64_t mCopyFenceValue;

    RenderContext mGraphicContext;
    ComPtr<ID3D12CommandQueue> mMainGraphicQueue;
    Fence mGraphicFence;
    uint64_t mFrameFenceValue;
    // ---------------------------------------------------------------
};

inline void D3dRenderer::updatePassConstants(uint8_t registerIndex, void* pData, uint64_t size)
{
#if defined(DEBUG) || defined(_DEBUG)
    ASSERT(registerIndex < GraphicSetting::gNumPassConstants, TEXT("register of pass constants buffer exceeded."));
#endif
    auto& buffer = mPassConstantsData[registerIndex];
    buffer.reserve(size);
    buffer.copyFrom(pData, size);
    mNumDirtyFrames = 3;
}
#endif
