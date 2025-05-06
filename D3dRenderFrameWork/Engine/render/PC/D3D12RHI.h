#pragma once
#ifdef WIN32
#include "Engine/memory/DynamicLinearAllocator.h"
#include "Engine/render/DynamicRHI.h"
#include "Private/D3D12CommandContext.h"

class D3D12Fence;
class D3D12RingBufferAllocator;
class D3D12PipelineStateManager;
class D3D12BuddyCBufferAllocator;
class RingDescriptorAllocator;
class BlockDescriptorAllocator;
class D3D12CommandObjectPool;

class D3D12RHI : public RHI
{
public:
    void Initialize(const RHIConfiguration& configuration) override;
    std::unique_ptr<RHIShader>          RHICompileShader(const Blob& binary, ShaderType activeTypes, const std::string* path = nullptr) override;
    std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingBuffer(uint64_t size) override;
    std::unique_ptr<RHIStagingBuffer>   RHIAllocStagingTexture(uint64_t size) override;
    std::unique_ptr<RHIConstantBuffer>  RHIAllocConstantBuffer(uint64_t size) override;
    std::unique_ptr<RHINativeTexture>   RHIAllocTexture(RHITextureDesc desc) override;
    std::unique_ptr<RHIDepthStencil>    RHIAllocDepthStencil(RHITextureDesc desc) override;
    std::unique_ptr<RHIRenderTarget>    RHIAllocRenderTarget(RHITextureDesc desc) override;
    std::unique_ptr<RHIVertexBuffer>    RHIAllocVertexBuffer(uint64_t vertexSize, uint64_t numVertices) override;
    std::unique_ptr<RHIIndexBuffer>     RHIAllocIndexBuffer(uint64_t numIndices, Format indexFormat) override;
    // std::unique_ptr<RHIFrameBuffer> RHICreateDepthStencilBuffer(RHINativeTexture* pDepthStencilBuffer) override;
    // std::unique_ptr<RHIFrameBuffer> RHICreateRenderTargetBuffer(RHINativeTexture* pRenderTargetBuffer) override;
    std::unique_ptr<RHIFence>           RHICreateFence() override;
    std::unique_ptr<RHISwapChain>       RHICreateSwapChain(const RHISwapChainDesc& desc) override;

    void UpdateStagingBuffer(RHIStagingBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) override;
    void UpdateConstantBuffer(RHIConstantBuffer* pBuffer, const void* pData, uint64_t offset, uint64_t size) override;
    void CreateGraphicsContext(RHIGraphicsContext** ppContext) override;
    void CreateCopyContext(RHICopyContext** ppContext) override;
    void CreateComputeContext(RHIComputeContext** ppContext) override;
    void ResetGraphicsContext(RHIGraphicsContext* pContext) override;
    void ResetCopyContext(RHICopyContext* pContext) const override;
    void SubmitRenderCommands(RHIGraphicsContext* pContext) override;
    void SubmitCopyCommands(RHICopyContext* pContext) override;
    void SyncGraphicContext(RHIFence* pFence, uint64_t semaphore) override;
    void SyncCopyContext(RHIFence* pFence, uint64_t semaphore) const override;
    void BatchCopyCommands(RHICopyContext** pContexts, uint32_t numContexts) override;
    void ReleaseGraphicsContext(RHIGraphicsContext* pContext) override;
    void ReleaseCopyContext(RHICopyContext* pContext) override;
    //void Present(RHISwapChain* pSwapChain) override;
    
    void Release() override;
    
    D3D12Device* GetD3D12Device() const;
    
    D3D12RHI();
    ~D3D12RHI() override;

private:
    //struct BatchTask
    //{
	   // BatchTask() = default;

	   // BatchTask(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* allocators[],
	   //           D3D12Fence* fence, uint64_t semaphore)
		  //  : mType(type),
		  //    mAllocators(allocators),
		  //    mFence(fence),
		  //    mSemaphore(semaphore)
	   // {
	   // }

	   // D3D12_COMMAND_LIST_TYPE mType;
    //    std::unique_ptr<ID3D12CommandAllocator*[]> mAllocators;
    //    D3D12Fence* mFence;
    //    uint64_t mSemaphore;
    //};

    static void GetShaderProperties(ShaderType type, ID3D12ShaderReflection* pReflector, std::unordered_set<ShaderProp>& properties);
    static void GetShaderInputElements(ID3D12ShaderReflection* pReflector, std::vector<ShaderInput>& inputElements);
    static Format GetFormatFromSignature(const D3D12_SIGNATURE_PARAMETER_DESC& paramDesc);
    void BuildGlobalRootSignature(std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE1[]>& ranges, std::vector<CD3DX12_ROOT_PARAMETER1>& params, std::vector<
                                         D3D12_STATIC_SAMPLER_DESC>& samplers) const;  // TEMP
    static std::vector<D3D12_STATIC_SAMPLER_DESC> GetStaticSamplers();

    std::unique_ptr<D3D12Device> mDevice;
    DynamicLinearAllocator      mShaderBinaryAllocator;
    
    std::unique_ptr<BlockDescriptorAllocator>       mRTVAllocator;
    std::unique_ptr<BlockDescriptorAllocator>       mDSVAllocator;
    std::unique_ptr<RingDescriptorAllocator>        mOnlineCBVSRVUAVAllocator;

    std::unique_ptr<D3D12RingBufferAllocator>       mConstantBufferAllocator;
    std::unique_ptr<D3D12RingBufferAllocator>       mStagingBufferAllocator;
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
