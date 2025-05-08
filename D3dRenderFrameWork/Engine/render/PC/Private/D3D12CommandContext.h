#pragma once
#ifdef WIN32
#include "ResourceStateTracker.h"
#include "Engine/render/PC/Native/D3D12CommandObjectPool.h"
#include "Engine/render/PC/Native/D3D12DescriptorAllocator.h"

class D3D12ConstantBuffer;
class D3D12RingBufferAllocator;
struct PipelineInitializer;
struct D3D12RootSignature;
class D3D12RootSignatureManager;
class D3D12PipelineStateManager;
struct TextureCopyLocation;

class D3D12CommandContext
{
    friend class D3D12RHI;
public:
    static void CopyBuffer(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst, const D3D12Resource* pSrc, uint64_t size, uint64_t dstStart, uint64_t srcStart);
    static void CopyTexture(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst, const D3D12Resource* pSrc, uint32_t baseSubResourceIndex,
                            uint32_t numSubResources);
    static void CopyTexture(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst, const D3D12Resource* pSrc, const TextureCopyLocation&
                            dstLocation, const TextureCopyLocation& srcLocation, uint32_t width, uint32_t height, uint32_t depth = 1);
    static void CopyTexture(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst, const D3D12Resource* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation);

    void Initialize(D3D12Device* pDevice, RingDescriptorAllocator* pOnlineDescriptorAllocator, D3D12RingBufferAllocator*
                    pRingCBufferAllocator, D3D12PipelineStateManager* pPSOManager, D3D12RootSignatureManager* pRootSigManager);
    //void Initialize2(ID3D12CommandQueue* directQueue, ID3D12CommandQueue* copyQueue, ID3D12CommandQueue* computeQueue);
    std::unique_ptr<D3D12ConstantBuffer> AllocFrameConstantBuffer(uint16_t size) const;
    void AllocDescriptors(std::unique_ptr<D3D12DescriptorHandle[]>& pDescriptors, D3D12DescriptorHandle*& pTextures, const D3D12RootSignature* pRootSignature) const;
    ID3D12PipelineState* GetPipelineStateObject(
	    const D3D12RootSignature* pRootSignature,
	    const PipelineInitializer& pPipelineInitializer) const;
    void GetOnlineDescriptorHeaps(std::unique_ptr<ID3D12DescriptorHeap*[]>& ppOnlineDescriptorHeaps, uint16_t& numHeaps) const;
    const D3D12RootSignature* GetRootSignature(/*const std::string& name*/) const;
    //ID3D12CommandQueue* GetDirectQueue() const;
    //ID3D12CommandQueue* GetCopyQueue() const;
    //ID3D12CommandQueue* GetComputeQueue() const;
    D3D12Device* GetDevice() const;

    //void TransitionResource(const D3D12Resource* pResource, uint32_t subResource, ResourceState state);
    //void TransitionResource(const D3D12Resource* pResource, ResourceState state);

    D3D12CommandContext();

private:
    // global reference
    D3D12Device* mDevice;
    D3D12PipelineStateManager* mPSOManager;
    D3D12RootSignatureManager* mRootSignatureManager;
    
    RingDescriptorAllocator* mOnlineDescriptorAllocator;
    D3D12RingBufferAllocator* mRingFrameCBufferAllocator;

    UComPtr<ID3D12Resource> mDummyCBuffer;
    UComPtr<ID3D12Resource> mDummyTexture;

    //ID3D12CommandQueue* mDirectQueue;
    //ID3D12CommandQueue* mCopyQueue;
    //ID3D12CommandQueue* mComputeQueue;
};
#endif
