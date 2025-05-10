#pragma once
#ifdef WIN32
#include "Engine/Memory/DynamicLinearAllocator.h"
#include "Engine/Render/DynamicRHI.h"
#include "Engine/Render/PC/Private/D3D12CommandContext.h"

struct RootSignatureLayout;
class D3D12Fence;
class D3D12RingBufferAllocator;
class D3D12PipelineStateManager;
class D3D12BuddyBufferAllocator;
class RingDescriptorAllocator;
class BlockDescriptorAllocator;
class D3D12CommandObjectPool;

class D3D12RHI : public RHI
{
public:
    void Initialize() override;
    std::unique_ptr<RHIShader>          RHICompileShader(const Blob& binary, ShaderType activeTypes, const std::string* path = nullptr) override;
    std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingBuffer(uint64_t size) override;
    std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingBuffer(const RHITextureDesc& desc, uint8_t mipmap) override;
    std::unique_ptr<RHIConstantBuffer>  RHIAllocConstantBuffer(uint64_t size) override;
    std::unique_ptr<RHIInstanceBuffer>  RHIAllocInstanceBuffer(uint64_t size) override;
    std::unique_ptr<RHINativeTexture>   RHIAllocTexture(RHITextureDesc desc) override;
    std::unique_ptr<RHIDepthStencil>    RHIAllocDepthStencil(RHITextureDesc desc) override;
    std::unique_ptr<RHIRenderTarget>    RHIAllocRenderTarget(RHITextureDesc desc) override;
    std::unique_ptr<RHIVertexBuffer>    RHIAllocVertexBuffer(uint32_t vertexSize, uint32_t numVertices) override;
    std::unique_ptr<RHIIndexBuffer>     RHIAllocIndexBuffer(uint64_t numIndices, Format indexFormat) override;
    // std::unique_ptr<RHIFrameBuffer> RHICreateDepthStencilBuffer(RHINativeTexture* pDepthStencilBuffer) override;
    // std::unique_ptr<RHIFrameBuffer> RHICreateRenderTargetBuffer(RHINativeTexture* pRenderTargetBuffer) override;
    std::unique_ptr<RHIFence>           RHICreateFence() override;
    std::unique_ptr<RHISwapChain>       RHICreateSwapChain(const RHISwapChainDesc& desc) override;

    void RHIUpdateStagingBuffer(RHIStagingBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) override;
    void RHIUpdateStagingBuffer(RHIStagingBuffer* pBuffer, const RHITextureDesc& desc, const void* pData, uint8_t mipmap) override;
    void RHIUpdateConstantBuffer(RHIConstantBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) override;
    void RHIUpdateInstanceBuffer(RHIInstanceBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) override;
    void RHIReleaseConstantBuffer(RHIConstantBuffer* pBuffer) override;
    void RHIReleaseInstanceBuffer(RHIInstanceBuffer* pBuffer) override;

    void RHICreateGraphicsContext(RHIGraphicsContext** ppContext) override;
    void RHICreateCopyContext(RHICopyContext** ppContext) override;
    void RHICreateComputeContext(RHIComputeContext** ppContext) override;
    void RHIResetGraphicsContext(RHIGraphicsContext* pContext) override;
    void RHIResetCopyContext(RHICopyContext* pContext) const override;
    void RHISubmitRenderCommands(RHIGraphicsContext* pContext) override;
    void RHISubmitCopyCommands(RHICopyContext* pContext) override;
    void RHISyncGraphicContext(RHIFence* pFence, uint64_t semaphore) override;
    void RHISyncCopyContext(RHIFence* pFence, uint64_t semaphore) const override;
    void RHIBatchCopyCommands(RHICopyContext** pContexts, uint32_t numContexts) override;
    void RHIReleaseGraphicsContext(RHIGraphicsContext* pContext) override;
    void RHIReleaseCopyContext(RHICopyContext* pContext) override;
    //void Present(RHISwapChain* pSwapChain) override;
    
    void Release() override;
    
    D3D12Device* GetD3D12Device() const;
    
    D3D12RHI();
    ~D3D12RHI() override;

private:
    static void GetShaderProperties(ShaderType type, ID3D12ShaderReflection* pReflector, std::unordered_set<ShaderProp>& properties);
    static void GetShaderInputElements(ID3D12ShaderReflection* pReflector, std::vector<ShaderInput>& inputElements);
    static Format GetFormatFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
    static void BuildGlobalRootSignature(const RootSignatureLayout& layout, std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1[]>& ranges, std::vector<CD3DX12_ROOT_PARAMETER1>& params, std::vector<
	                                         D3D12_STATIC_SAMPLER_DESC>& samplers);  // TEMP
    static std::vector<D3D12_STATIC_SAMPLER_DESC> GetStaticSamplers();


    std::unique_ptr<D3D12Device> mDevice;
    DynamicLinearAllocator      mShaderBinaryAllocator;
    
    std::unique_ptr<BlockDescriptorAllocator>       mRTVAllocator;
    std::unique_ptr<BlockDescriptorAllocator>       mDSVAllocator;
    std::unique_ptr<RingDescriptorAllocator>        mOnlineCBVSRVUAVAllocator;

    std::unique_ptr<D3D12RingBufferAllocator>       mRingCBufferAllocator;  // for per object constants, frequently update(instance buffer)
    std::unique_ptr<D3D12BuddyBufferAllocator>      mBuddyCBufferAllocator; // for per material instance constants
    std::unique_ptr<D3D12RingBufferAllocator>       mStagingBufferAllocator;// for intermediate buffer used to upload data to default heap.
    std::unique_ptr<D3D12CommandObjectPool>         mCommandObjectPool;
    std::unique_ptr<D3D12RootSignatureManager>      mRootSignatureManager;
    std::unique_ptr<D3D12PipelineStateManager>      mPipelineStateManager;

    UComPtr<ID3D12CommandQueue> mDirectQueue;
    UComPtr<ID3D12CommandQueue> mCopyQueue;
    UComPtr<ID3D12CommandQueue> mComputeQueue;
    D3D12CommandContext mCommandContext;

    //std::unordered_map<uint64_t, std::vector<ID3D12CommandAllocator*>> mActiveAllocators;
};

#endif
