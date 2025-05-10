#ifdef WIN32
#include "D3D12CommandContext.h"

#include "Engine/Render/RHIDescriptors.h"
#include "Engine/Render/PC/Native/D3D12ConstantBufferAllocator.h"
#include "Engine/Render/PC/Native/D3D12PipelineStateManager.h"
#include "Engine/Render/PC/Native/D3D12Resource.h"
#include "Engine/Render/PC/Native/D3D12RootSignatureManager.h"
#include "Engine/Render/PC/Private/D3D12Resources.h"

void D3D12CommandContext::CopyBuffer(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst, const D3D12Resource* pSrc, uint64_t size,
                                     uint64_t dstStart, uint64_t srcStart)
{
    pCommandList->CopyBufferRegion(pDst->D3D12ResourcePtr(), dstStart, pSrc->D3D12ResourcePtr(), srcStart,
        size);

    // DynamicLinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyBufferRegion>();
    // D3D12CommandCopyBufferRegion* pCommand = new (hCommand.Get()) D3D12CommandCopyBufferRegion{};
    // pCommand->SetCopyParameters(static_cast<D3D12Buffer*>(pDst)->D3D12ResourcePtr(), static_cast<D3D12Buffer*>(pSrc)->D3D12ResourcePtr(), mSize, dstStart, srcStart);
    // mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::CopyTexture(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst,
    const D3D12Resource* pSrc, uint32_t baseSubResourceIndex, uint32_t numSubResources)
{
    ID3D12Resource* pDstResource = pDst->D3D12ResourcePtr();
    ID3D12Resource* pSrcResource = pSrc->D3D12ResourcePtr();
    D3D12_TEXTURE_COPY_LOCATION srcCopyLocation;
    srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcCopyLocation.pResource = pSrcResource;
    srcCopyLocation.SubresourceIndex = 0;
    D3D12_TEXTURE_COPY_LOCATION dstCopyLocation;
    dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstCopyLocation.pResource = pDstResource;

    uint32_t upper = baseSubResourceIndex + numSubResources;

    for (dstCopyLocation.SubresourceIndex = baseSubResourceIndex; dstCopyLocation.SubresourceIndex < upper; ++
        dstCopyLocation.SubresourceIndex)
    {
        pCommandList->CopyTextureRegion(
            &dstCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
    }

    // DynamicLinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyTextureRegion>();
    // D3D12CommandCopyTextureRegion* pCommand = new (hCommand.Get()) D3D12CommandCopyTextureRegion{};
    // pCommand->SetCopyParameters(srcBox, srcLocation, dstLocation, dstX, dstY, dstZ);
    // mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::CopyTexture(ID3D12GraphicsCommandList* pCommandList, const D3D12Resource* pDst,
    const D3D12Resource* pSrc, const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation,
    uint32_t width, uint32_t height, uint32_t depth)
{
    ID3D12Resource* pDstResource = pDst->D3D12ResourcePtr();
    ID3D12Resource* pSrcResource = pSrc->D3D12ResourcePtr();
    D3D12_RESOURCE_DESC dstDesc = pDstResource->GetDesc();
    D3D12_RESOURCE_DESC srcDesc = pSrcResource->GetDesc();
    uint32_t srcSubResource = srcLocation.mMipmap + srcDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D || srcDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ?
        0 : srcLocation.mArrayIndex * srcDesc.MipLevels;
    uint32_t dstSubResource = dstLocation.mMipmap + dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D || dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ?
        0 : dstLocation.mArrayIndex * dstDesc.MipLevels;
    D3D12_TEXTURE_COPY_LOCATION srcCopyLocation;
    srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcCopyLocation.pResource = pSrcResource;
    srcCopyLocation.SubresourceIndex = srcSubResource;
    D3D12_TEXTURE_COPY_LOCATION dstCopyLocation;
    dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstCopyLocation.pResource = pDstResource;
    dstCopyLocation.SubresourceIndex = dstSubResource;
    D3D12_BOX srcBox{ srcLocation.mPosX, srcLocation.mPosY, srcLocation.mPosZ, srcLocation.mPosX + width, srcLocation.mPosY + height, srcLocation.mPosZ + depth };

    pCommandList->CopyTextureRegion(
        &dstCopyLocation, dstLocation.mPosX, dstLocation.mPosY, dstLocation.mPosZ, &srcCopyLocation, &srcBox);

    // DynamicLinearAllocator::Handle<RHICommand> hCommand = mAllocator.Allocate<RHICommand, D3D12CommandCopyTextureRegion>();
    // D3D12CommandCopyTextureRegion* pCommand = new (hCommand.Get()) D3D12CommandCopyTextureRegion{};
    // pCommand->SetCopyParameters(srcBox, srcLocation, dstLocation, dstX, dstY, dstZ);
    // mCommandsBuffer.emplace_back(hCommand);
}

void D3D12CommandContext::CopyTexture(ID3D12GraphicsCommandList* pCommandList,
    const D3D12Resource* pDst, const D3D12Resource* pSrc,
    const TextureCopyLocation& dstLocation, const TextureCopyLocation& srcLocation)
{
    ID3D12Resource* pDstResource = pDst->D3D12ResourcePtr();
    ID3D12Resource* pSrcResource = pSrc->D3D12ResourcePtr();
    D3D12_RESOURCE_DESC dstDesc = pDstResource->GetDesc();
    D3D12_RESOURCE_DESC srcDesc = pSrcResource->GetDesc();

    if (srcDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER &&
        srcDesc.Dimension != dstDesc.Dimension) {
        // ������
        return;
    }

    D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = {};
    srcCopyLocation.pResource = pSrcResource;

    if (srcDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
        
        uint32_t srcSubResource = srcLocation.mMipmap +
            ((srcDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) ?
                (srcLocation.mArrayIndex * srcDesc.MipLevels) : 0);

        srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcCopyLocation.SubresourceIndex = srcSubResource;
    }
    else {
        ID3D12Device* pDevice;
        pCommandList->GetDevice(IID_PPV_ARGS(&pDevice));

        uint32_t dstSubResource = dstLocation.mMipmap +
            ((dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) ?
                (dstLocation.mArrayIndex * dstDesc.MipLevels) : 0);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
        UINT numRows;
        UINT64 rowSize, totalBytes;
        pDevice->GetCopyableFootprints(&dstDesc, dstSubResource, 1,
            0, &footprint, &numRows, &rowSize, &totalBytes);

        srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcCopyLocation.PlacedFootprint = footprint;
        srcCopyLocation.PlacedFootprint.Offset += srcLocation.mPosX;

        pDevice->Release();
    }

    uint32_t dstSubResource = dstLocation.mMipmap +
        ((dstDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) ?
            (dstLocation.mArrayIndex * dstDesc.MipLevels) : 0);

    D3D12_TEXTURE_COPY_LOCATION dstCopyLocation = {};
    dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstCopyLocation.pResource = pDstResource;
    dstCopyLocation.SubresourceIndex = dstSubResource;

    pCommandList->CopyTextureRegion(
        &dstCopyLocation,
        dstLocation.mPosX, dstLocation.mPosY, dstLocation.mPosZ,
        &srcCopyLocation,
        nullptr);
}

void D3D12CommandContext::Initialize(D3D12Device* pDevice,
                                     RingDescriptorAllocator* pOnlineDescriptorAllocator, D3D12RingBufferAllocator* pRingCBufferAllocator, D3D12PipelineStateManager* pPSOManager, D3D12RootSignatureManager*
                                     pRootSigManager)
{
    ASSERT(pDevice, TEXT("device must not be null."));
    ASSERT(pOnlineDescriptorAllocator, TEXT("OnlineDescriptorAllocator must not be null."));
    
    mDevice = pDevice;
    mOnlineDescriptorAllocator = pOnlineDescriptorAllocator;
    mPSOManager = pPSOManager;
    mRootSignatureManager = pRootSigManager;
    mRingBufferAllocator = pRingCBufferAllocator;

    mDummyCBuffer = mDevice->CreateCommitedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, CD3DX12_RESOURCE_DESC::Buffer(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT),
        nullptr, D3D12_RESOURCE_STATE_COMMON);
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(1);
    //desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN
    mDummyTexture = mDevice->CreateCommitedResource(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, desc,
        nullptr, D3D12_RESOURCE_STATE_COMMON);
}

std::unique_ptr<D3D12UploadBuffer> D3D12CommandContext::AllocTemporaryBuffer(uint32_t size, uint32_t stride) const
{
    return std::make_unique<D3D12UploadBuffer>(mRingBufferAllocator->GetD3D12Resource(),
        mRingBufferAllocator->Allocate(
            ::AlignUpToMul<uint64_t, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>()(size)), ::AlignUpToMul<uint32_t, sizeof(float[4])>()(stride));
}

//void D3D12CommandContext::Initialize2(ID3D12CommandQueue* directQueue, ID3D12CommandQueue* copyQueue,
//                                      ID3D12CommandQueue* computeQueue)
//{
//    mDirectQueue = directQueue;
//    mCopyQueue = copyQueue;
//    mComputeQueue = computeQueue;
//}

void D3D12CommandContext::AllocDescriptors(std::unique_ptr<D3D12DescriptorHandle[]>& pDescriptors,
                                           D3D12DescriptorHandle*& pTextures, const D3D12RootSignature* pRootSignature) const
{
    ASSERT(pRootSignature, TEXT("invalid root signature."));

    const RootSignatureLayout& layout = pRootSignature->mLayout;
    pDescriptors = mOnlineDescriptorAllocator->Allocate(layout.mNumTextures + layout.mNumMaterialConstants);
    pTextures = pDescriptors.get() + layout.mNumMaterialConstants;
    for (int i = 0; i < layout.mNumMaterialConstants; ++i)
    {
        static D3D12_CONSTANT_BUFFER_VIEW_DESC desc = { mDummyCBuffer->GetGPUVirtualAddress(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT };
        mDevice->CreateConstantBufferView(desc, pDescriptors[i].mCPUHandle);
    }
    for (int i = 0; i < layout.mNumTextures; ++i)
    {
        static constexpr D3D12_SHADER_RESOURCE_VIEW_DESC desc = { DXGI_FORMAT_UNKNOWN, D3D12_SRV_DIMENSION_BUFFER, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, D3D12_BUFFER_SRV{0, 1, 1, D3D12_BUFFER_SRV_FLAG_NONE}};
        mDevice->CreateShaderResourceView(mDummyTexture.Get(), desc, pTextures[i].mCPUHandle);
    }
}

void D3D12CommandContext::GetOnlineDescriptorHeaps(std::unique_ptr<ID3D12DescriptorHeap*[]>& ppOnlineDescriptorHeaps, uint16_t& numHeaps) const
{
    //for (uint32_t i = 0; i < layout.mNumMaterialConstants - 2; i++)
    //{
    //    Blob& constant = *mConstantHandles[i];
    //    D3D12PooledBuffer&& allocation = mRingBufferAllocator.Allocate(::AlignUpToMul<uint64_t, 256>()(constant.Size()));
    //    memcpy(allocation.mCPUAddress, constant.Binary(), constant.Size());
    //    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {allocation.mGPUAddress, static_cast<UINT32>(allocation.mSize)};
    //    mDevice->CreateConstantBufferView(cbvDesc, handles[layout.mNumTextures + i].mCPUHandle);
    //    mAllocatedCBSize += allocation.mSize;
    //}
    ppOnlineDescriptorHeaps.reset(new ID3D12DescriptorHeap*[1]{ mOnlineDescriptorAllocator->GetHeap()->GetD3D12DescriptorHeap() });
    numHeaps = 1;
}

D3D12Device* D3D12CommandContext::GetDevice() const
{
    return mDevice;
}

const D3D12RootSignature* D3D12CommandContext::GetRootSignature() const
{
    // TODO: query root signature by name or index.
    return mRootSignatureManager->GetByIndex(0);
}

//ID3D12CommandQueue* D3D12CommandContext::GetDirectQueue() const
//{
//    return mDirectQueue;
//}
//
//ID3D12CommandQueue* D3D12CommandContext::GetCopyQueue() const
//{
//    return mCopyQueue;
//}
//
//ID3D12CommandQueue* D3D12CommandContext::GetComputeQueue() const
//{
//    return mComputeQueue;
//}

ID3D12PipelineState* D3D12CommandContext::GetPipelineStateObject(
	const D3D12RootSignature* pRootSignature,
	const PipelineInitializer& pPipelineInitializer) const
{
    return mPSOManager->GetOrCreateGraphicsPSO(pRootSignature->mRootSignature.Get(), pPipelineInitializer);
}

D3D12CommandContext::D3D12CommandContext() = default;
#endif
