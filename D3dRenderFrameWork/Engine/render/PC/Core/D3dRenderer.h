#pragma once
#include "D3dCommandListPool.h"
#include "Engine/render/PC/Resource/D3dAllocator.h"
#include "Engine/render/PC/Resource/StaticBuffer.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/common/helper.h"
#include "Engine/render/Renderer.h"
#include "Engine/render/PC/Resource/RenderTexture.h"
#include "Engine/render/PC/Core/DescriptorHeap.h"


// SampleRenderPath* gSampleRenderPath();

struct ShaderConstant;
struct RenderList;
class Shader;
class D3dContext;

struct RenderData
{
    std::vector<DynamicBuffer*> mConstantsBuffers;
};

struct GraphicSetting
{
    TextureFormat mBackBufferFormat = TextureFormat::R8G8B8A8_UNORM;
    TextureFormat mDepthStencilFormat = TextureFormat::D24_UNORM_S8_UINT;
    DXGI_SAMPLE_DESC mSampleDesc = {1, 0};
    
    uint64_t mNumPassConstants = 2;
    uint64_t mNumPerObjectConstants = 2;
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
    
    void registerShaders(const std::vector<Shader*>& shaders);
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<D3dResource, T>>> ResourceHandle allocateBuffer(uint64_t size);
    template<typename T, typename = std::enable_if_t<std::is_base_of_v<D3dResource, T>>> void updateResource(const ResourceHandle& resourceHandle, const void* data) const;
    void releaseResource(const ResourceHandle& resourceHandle) const;
    void updatePassConstants(uint8_t registerIndex, void* pData, uint64_t size);
    void appendRenderLists(std::vector<RenderList>&& renderLists);
    void render();
    void release();
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
    void initializeImpl(HWND hWindow);
    void createRootSignature();
    D3dRenderer();

    ID3D12RootSignature* mGlobalRootSignature;

    std::thread::id mRenderingThreadId;

    std::unique_ptr<DescriptorHeap> mRtDescHeap;
    std::unique_ptr<DescriptorHeap> mDsDescHeap;
    std::unique_ptr<DescriptorHeap> mCbSrUaDescHeap;
    
    ComPtr<IDXGISwapChain1> mSwapChain;
    ComPtr<ID3D12CommandQueue> mMainComputeQueue;
    
    RenderTexture2D* mBackBuffers;
    RenderTexture2D mDepthStencilBuffer;
    // FrameResource* mFrameResources;
    uint8_t mCpuWorkingPageIdx;
    
    std::unique_ptr<D3dContext> mD3dContext;
    GraphicSetting mGraphicSettings;
    // std::unordered_map<RenderPath*, RenderData**> mRenderPaths;

    D3dAllocator mAllocator;
    
    std::unordered_map<Shader*,ID3D12PipelineState*, HashPtrAsTyped<Shader*>> mPipelineStates;
    std::vector<RenderList> mPendingRenderLists;

    // std::vector<D3dResource*>* mConstantBuffers;
    std::vector<void*> mPassConstantsData;
    RenderData* mRenderData;
    uint8_t mNumDirtyFrames;
    std::vector<D3dResource*> mResources;
    std::stack<uint64_t> mAvailableResourceAddresses;
    std::vector<uint64_t>* mReleasingResources;

    RenderContext mCopyContext;
    ComPtr<ID3D12CommandQueue> mCopyQueue;
    Fence mCopyFence;
    uint64_t mCopyFenceValue;

    RenderContext mGraphicContext;
    ComPtr<ID3D12CommandQueue> mMainGraphicQueue;
    Fence mGraphicFence;
    uint64_t mFrameFenceValue;
};

inline void D3dRenderer::updatePassConstants(uint8_t registerIndex, void* pData, uint64_t size)
{
#if defined(DEBUG) || defined(_DEBUG)
    ASSERT(registerIndex < mGraphicSettings.mNumPassConstants, TEXT("register of pass constants buffer exceeded."));
#endif
    byte* buffer = new byte[size];
    memcpy(buffer, pData, size);
    delete[] static_cast<byte*>(mPassConstantsData[registerIndex]);
    mPassConstantsData[registerIndex] = buffer;
    mNumDirtyFrames = 3;
}

//
// template<typename T, typename = void>
// struct CreateRenderPath;
//
// template<typename T>
// struct CreateRenderPath<T, std::enable_if_t<std::is_base_of_v<RenderPath, T>>>
// {
//     T* operator()()
//     {
//         return new T();
//     }
// };

// template <typename T>
// void D3dRenderer::registerRenderPath()
// {
//     // T* pPath = CreateRenderPath<T>()();
//     // D3dAllocator allocator{mD3dContext.get()};
//     // RenderData** bufferPtrs = new RenderData*[mGraphicSettings.mNumBackBuffers];
//     // pPath->setUp(*mD3dContext);
//     // // mRenderPaths.insert(new ForwardRenderPath());
//     // for (int i = 0; i < mGraphicSettings.mNumBackBuffers; ++i)
//     // {
//     //     bufferPtrs[i] = pPath->allocResources(allocator);
//     // }
//     // mRenderPaths.emplace(pPath, bufferPtrs);
// }

template <>
inline ResourceHandle D3dRenderer::allocateBuffer<DynamicBuffer>(uint64_t size)
{
    auto dynamicBuffer = mAllocator.allocDynamicBuffer(size);
    if (mAvailableResourceAddresses.empty())
    {
        mResources.push_back(dynamicBuffer);
        return { mResources.size() - 1 };
    }
    ResourceHandle resourceHandle = { mAvailableResourceAddresses.top() };
    mResources[resourceHandle.mIndex] = dynamicBuffer;
    return resourceHandle;
}

template <>
inline ResourceHandle D3dRenderer::allocateBuffer<StaticBuffer>(uint64_t size)
{
    auto staticBuffer = mAllocator.allocStaticBuffer(size);
    if (mAvailableResourceAddresses.empty())
    {
        mResources.push_back(staticBuffer);
        return { mResources.size() - 1 };
    }
    ResourceHandle resourceHandle = { mAvailableResourceAddresses.top() };
    mResources[resourceHandle.mIndex] = staticBuffer;
    return resourceHandle;
}

template <>
inline void D3dRenderer::updateResource<DynamicBuffer>(const ResourceHandle& resourceHandle, const void* data) const
{
    DynamicBuffer* stagingBuffer = dynamic_cast<DynamicBuffer*>(mResources[resourceHandle.mIndex]);
    memcpy(stagingBuffer->mappedPointer(), data, stagingBuffer->size());
}

template <>
inline void D3dRenderer::updateResource<StaticBuffer>(const ResourceHandle& resourceHandle, const void* data) const
{
    D3dCommandList* pCommandList = D3dCommandListPool::getCommandList(D3dCommandListType::COPY);
    StaticBuffer* staticBuffer = dynamic_cast<StaticBuffer*>(mResources[resourceHandle.mIndex]);
    uint64_t size = staticBuffer->size(); 
    DynamicBuffer* stagingBuffer = mAllocator.allocDynamicBuffer(size);
    memcpy(stagingBuffer->mappedPointer(), data, size);
    pCommandList->copyResource(*staticBuffer, *stagingBuffer);
    pCommandList->close();
    mCopyContext.executeCommandList(pCommandList);
    D3dCommandListPool::recycle(pCommandList);
}

inline void D3dRenderer::releaseResource(const ResourceHandle& resourceHandle) const
{
    mReleasingResources[mGraphicSettings.mNumBackBuffers].push_back(resourceHandle.mIndex);
}
#endif
