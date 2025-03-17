#pragma once

#ifdef WIN32
#include <pch.h>
#include <Engine/render/Renderer.h>

class D3dRenderer;
class Shader;
class ResourceStateTracker;
class FrameResource;
class WFrame;
class D3dCommandList;
class D3dGraphicContext;
class DescriptorHeap;
struct Fence;

const Shader& TempOpaquePs();
const Shader& TempOpaqueVs();

D3dRenderer* gNativeRenderer();
D3dGraphicContext const* gGraphicContext();

class D3dRenderer : public Renderer
{
    friend D3dRenderer* ::gNativeRenderer();
	friend D3dGraphicContext const* ::gGraphicContext();

public:
    static Fence sCreateFence();
    static DescriptorHeap sCreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type,
                                               uint16_t size);

    ~D3dRenderer() override;
    
    DELETE_COPY_OPERATOR(D3dRenderer)
    DELETE_COPY_CONSTRUCTOR(D3dRenderer)
    DELETE_MOVE_OPERATOR(D3dRenderer)
    DELETE_MOVE_CONSTRUCTOR(D3dRenderer)

private:
    constexpr static uint16_t MAX_RENDER_TARGET_COUNT = 8;
    constexpr static uint16_t DEPTH_STENCIL_COUNT = 1;
    constexpr static uint16_t MAX_CB_SR_UA_RESOURCE_COUNT = 256;

#pragma region Temp
    constexpr static uint16_t PASS_COUNT = 1;
    constexpr static uint16_t OBJECT_COUNT = 1;
#pragma endregion
    
#pragma region Settings
    uint8_t mBackbufferCount = 3;
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
#pragma endregion

    void Initialize();
    void CreateDescriptorHeap();
    void CreateCommandQueue();
    void LoadShaders();
    void CreateSwapChainForHwnd();
    void CreateBackBuffersAndDesc();
    void CreateDepthStencilAndDesc();
    void CreateFrameResources();
    void CreateRootSignature();
    
    D3dRenderer();

    static std::unique_ptr<D3dGraphicContext> sRenderContext;
    static std::unique_ptr<WFrame> sFrame;

    ComPtr<IDXGISwapChain1> mSwapChain;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12Resource>* mBackBuffers;
    ComPtr<ID3D12Resource> mDepthStencilBuffer;
    std::unique_ptr<FrameResource>* mFrameResources;
    uint8_t mCurrentBackBufferIndex;
    
    ComPtr<ID3D12RootSignature> mRootSignature;
    std::unique_ptr<DescriptorHeap> mRtDescHeap;
    std::unique_ptr<DescriptorHeap> mDsDescHeap;
    std::unique_ptr<DescriptorHeap> mCbSrUaDescHeap;
    std::unique_ptr<Fence> mFence;
};
#endif