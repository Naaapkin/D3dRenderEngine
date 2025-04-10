#pragma once
#include "Engine/pch.h"
#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"
#include "Engine/render/RHIDefination.h"
#include "Engine/render/PC/D3dUtil.h"
#include "Engine/render/PC/Core/D3D12Device.h"
#include "Engine/render/PC/Core/D3dObject.h"
#include "Engine/render/PC/Core/ResourceStateTracker.h"

class D3D12Resource: public D3D12DeviceChild
{
    friend struct std::hash<D3D12Resource>;
public:
    virtual void CreateD3D12Resource(D3D12Device* parent, const D3D12_HEAP_PROPERTIES& heapProp,
        const D3D12_HEAP_FLAGS heapFlags,
        const D3D12_RESOURCE_DESC& desc,
        const D3D12_CLEAR_VALUE& clearValue,
        D3D12_RESOURCE_STATES initialState)
    {
        CreateD3D12Object(parent);
        mResource = parent->CreateCommitedResource(heapProp, heapFlags, desc, clearValue, initialState);
        mGPUHandle = MAXUINT64;
    }
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return mResource->GetGPUVirtualAddress(); }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescHandle() const { return { mGPUHandle }; }
    void BindDescHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { mGPUHandle = handle.ptr; }
    uint32_t GetSubResourceCount() const
    {
        D3D12_RESOURCE_DESC desc = mResource->GetDesc();
        return static_cast<uint32_t>(desc.MipLevels) * desc.DepthOrArraySize * D3D12GetFormatPlaneCount(Device(), desc.Format);
    }
    ID3D12Resource* NativeResource() const { return mResource; }
    void* MappedPtr() const
    {
        ASSERT(mResourceType == ResourceType::DYNAMIC, TEXT("can't map non dynamic buffer."))
        return mMappedPtr;
    }
    ResourceType GetResourceType() const { return mResourceType; }
    D3D12Resource() : mResourceType(ResourceType::NONE), mResource(nullptr), mGPUHandle(MAXUINT64), mMappedPtr(nullptr) { }
    D3D12Resource(D3D12Device* parent, ID3D12Resource* pResource, ResourceType resourceType) : D3D12DeviceChild(parent),
        mResourceType(resourceType), mResource(pResource), mGPUHandle(MAXUINT64)
    {
        if (mResourceType == ResourceType::DYNAMIC)
        {
            D3D12_RANGE readRange = { 0, 0 };
            ThrowIfFailed(mResource->Map(0, &readRange, &mMappedPtr));
        }
    }
    ~D3D12Resource() override
    {
        if (mMappedPtr) mResource->Unmap(0, nullptr);
    }

private:
    ResourceType mResourceType;
    ID3D12Resource* mResource;
    uint64_t mGPUHandle;
    void* mMappedPtr;
};

template<>
struct std::hash<D3D12Resource>
{
    size_t operator()(const D3D12Resource& resource) const noexcept
    {
        return std::hash<uint64_t>()(reinterpret_cast<uint64_t>(resource.mResource));
    }
};

class D3D12Buffer final : public D3D12Resource, public RHIBuffer 
{
public:
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    RHIBufferDesc GetDesc() const override
    {
        D3D12_RESOURCE_DESC d3d12Desc = NativeResource()->GetDesc();
        return { d3d12Desc.Width, d3d12Desc.Alignment, GetResourceType() };
    }
    uint64_t GetSize() const override
    {
        return NativeResource()->GetDesc().Width;
    }

    D3D12Buffer() { mName = "<unknown>"; }
    D3D12Buffer(D3D12Device* parent, ID3D12Resource* pResource, ResourceType type) : D3D12Resource(parent, pResource, type) { mName = "<unknown>"; }
};

class D3D12Texture : public D3D12Resource
{
public:
    D3D12Texture()  { mName = "<unknown>"; }
    D3D12Texture(D3D12Device* parent, ID3D12Resource* pResource, ResourceType resourceType) : D3D12Resource(parent, pResource, resourceType) { mName = "<unknown>"; }
};

class D3D12Texture2D final : public RHITexture2D, public D3D12Texture
{
public:
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    void GetSize(uint32_t& width, uint32_t& height) const override
    {
        D3D12_RESOURCE_DESC d3d12Desc = NativeResource()->GetDesc();
        width = d3d12Desc.Width; height = d3d12Desc.Height;
    }
    const RHITextureDesc& GetDesc() const override
    {
        D3D12_RESOURCE_DESC d3d12Desc = NativeResource()->GetDesc();
        return {
            static_cast<Format>(d3d12Desc.Format), TextureDimension::TEXTURE2D, static_cast<uint32_t>(d3d12Desc.Width),
            d3d12Desc.Height, 1, static_cast<uint8_t>(d3d12Desc.MipLevels),
            static_cast<uint8_t>(d3d12Desc.SampleDesc.Count), static_cast<uint8_t>(d3d12Desc.SampleDesc.Quality)
        };
    }
};

class D3D12DescriptorHeap final : D3D12DeviceChild
{
public:    
    void GrowIfNeeded(uint32_t numDescriptors)
    {
        ID3D12Device* pDevice = Device();
        uint32_t cap = mDescHeap->GetDesc().NumDescriptors;
        if (cap < numDescriptors)
        {
            ID3D12DescriptorHeap* pNewHeap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = mDescHeap->GetDesc();
            desc.NumDescriptors = ::AlignUpToMul<uint64_t, GROW_BLOCK_SIZE>()(numDescriptors);
            pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pNewHeap));
            pDevice->CopyDescriptorsSimple(cap, pNewHeap->GetCPUDescriptorHandleForHeapStart(),
                                           mDescHeap->GetCPUDescriptorHandleForHeapStart(), desc.Type);
            mDescHeap.Release();
            mDescHeap.Reset(pNewHeap);
        }
    }
    bool IsGPUVisible() const
    {
        return mDescHeap->GetDesc().Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; 
    }
    D3D12_DESCRIPTOR_HEAP_DESC GetDescHeapDesc() const
    {
        return mDescHeap->GetDesc();
    }
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(uint64_t index) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * mDescSize;
        return handle;
    }
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(uint64_t index) const
    {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = mDescHeap->GetGPUDescriptorHandleForHeapStart();
        handle.ptr += index * mDescSize;
        return handle;
    }
    uint64_t DescriptorSize() const { return mDescSize; }
    D3D12DescriptorHeap() = default;
    D3D12DescriptorHeap(D3D12Device* parent, ID3D12DescriptorHeap* pHeap) : D3D12DeviceChild(parent), mDescHeap(pHeap), mDescSize(Device()->GetDescriptorHandleIncrementSize(pHeap->GetDesc().Type)) { }

private:
    static constexpr uint64_t GROW_BLOCK_SIZE = 64;
    
    UComPtr<ID3D12DescriptorHeap> mDescHeap;
    uint64_t mDescSize = 0;
};

class D3D12Fence final : D3D12DeviceChild, public RHIFence
{
public:
    ID3D12Fence* NativeFence() const { return mFence.Get(); }
    void SetName(const std::string& name) override { mName = name; };
    const std::string& GetName() const override { return mName; }
    void Wait(uint64_t fencePoint) const override
    {
        if (mFence->GetCompletedValue() < fencePoint)
        {
            const HANDLE he = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            ThrowIfFailed(
                mFence->SetEventOnCompletion(fencePoint, he)
            )
            WaitForSingleObject(he, INFINITE);
            CloseHandle(he);
        }
    }
    D3D12Fence() = default;
    D3D12Fence(D3D12Device* parent, ID3D12Fence* pFence) : D3D12DeviceChild(parent), mFence(pFence) { mName = "<unknown>"; }

private:
    UComPtr<ID3D12Fence> mFence;
};

class D3D12CommandQueue final : D3D12DeviceChild, public RHICommandQueue
{
public:
    ID3D12CommandQueue* NativeQueue() const { return mQueue.Get(); }
    D3D12_COMMAND_LIST_TYPE GetType() const { return mQueue->GetDesc().Type; }
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    void Signal(const RHIFence* pFence, uint64_t fenceValue) const override
    {
        mQueue->Signal(static_cast<const D3D12Fence*>(pFence)->NativeFence(), fenceValue);
    }
    void Wait(const RHIFence* pFence, uint64_t fenceValue) const override
    {
        mQueue->Wait(static_cast<const D3D12Fence*>(pFence)->NativeFence(), fenceValue);
    }
    void ExecuteCommandLists(RHIGraphicCommandList* commandLists, uint32_t numPendingLists) override;  // TODO:
    D3D12CommandQueue() = default;
    D3D12CommandQueue(D3D12Device* parent, ID3D12CommandQueue* pQueue);

private:
    UComPtr<ID3D12CommandQueue> mQueue;
};

class D3D12FrameResource : D3D12DeviceChild, public RHIFrameResource
{
public:
    void Synchronize(const D3D12Fence* pFence) { pFence->Wait(mFenceValue++); }
    uint64_t Semaphore() const { return mFenceValue; }

    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    const std::vector<D3D12Buffer>& GetPassCB() const { return mPassCbuffers; }
    const std::vector<D3D12Buffer>& GetMaterialCB() const { return mMaterialCBuffers; }
    void UpdatePassConstant(uint32_t slot, const Blob& data) override
    {
        void* ptr = mPassCbuffers[slot].MappedPtr();
        memcpy(ptr, data.binary(), data.size());
    }
    void UpdateMaterialConstant(uint32_t slot, const Blob& data) override
    {
        void* ptr = mMaterialCBuffers[slot].MappedPtr();
        memcpy(ptr, data.binary(), data.size());
    }
    void SetGlobalTextureResource(uint32_t slot, RHITexture* pTex) override
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = mOfflineHeap->CPUHandle(slot);
        Device()->CreateShaderResourceView(reinterpret_cast<D3D12Texture*>(pTex)->NativeResource(), nullptr, handle);
    }
    D3D12FrameResource() = default;
    D3D12FrameResource(D3D12Device* parent, std::vector<D3D12Buffer>& passCBuffers, std::vector<D3D12Buffer>& materialCBuffers) :
        D3D12DeviceChild(parent), mName("<unknown>"),
            mPassCbuffers(std::move(passCBuffers)),
            mMaterialCBuffers(std::move(materialCBuffers)),
            mFenceValue(0) { }

private:
    std::string mName;
    std::vector<D3D12Buffer> mPassCbuffers;
    std::vector<D3D12Buffer> mMaterialCBuffers;
    std::unique_ptr<D3D12DescriptorHeap> mOfflineHeap;
    uint64_t mFenceValue;
};

class D3D12CommandList : D3D12DeviceChild
{
public:
    void SetCommandAllocator(ID3D12CommandAllocator* pAllocator) { mCommandAllocator = pAllocator; }
    void SetCommandList(ID3D12GraphicsCommandList* pCommandList) { mCommandList = pCommandList; };
    ID3D12CommandAllocator* Allocator() const { return mCommandAllocator; }
    ID3D12GraphicsCommandList* NativeCommandList() const { return mCommandList; }

    D3D12CommandList() : mName("<unknown>"), mCommandList(nullptr), mCommandAllocator(nullptr) { }
    D3D12CommandList(D3D12Device* parent, ID3D12CommandAllocator* pAllocator, ID3D12GraphicsCommandList* pCommandList, const std::string& name) :
        D3D12DeviceChild(parent), mName(name), mCommandList(pCommandList), mCommandAllocator(pAllocator) { }
    
protected:
    std::string mName;
    ID3D12GraphicsCommandList* mCommandList;
    ID3D12CommandAllocator* mCommandAllocator;
};

class D3D12CopyCommandList : public D3D12CommandList, public RHICopyCommandList
{
public:
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    void Reset() override { mCommandList->Reset(mCommandAllocator, nullptr); }
    void Clear() override { mCommandList->ClearState(nullptr); }
    void Close() override { mCommandList->Close(); }
    void CopyResource(RHIResource* dstResource, RHIResource* srcResource) override;

    D3D12CopyCommandList() = default;
    D3D12CopyCommandList(D3D12Device* parent, ID3D12CommandAllocator* pAllocator,
                         ID3D12GraphicsCommandList* pCommandList, const std::string& name) : D3D12CommandList(
        parent, pAllocator, pCommandList, name) { }
};

class D3D12GraphicCommandList : public D3D12CommandList, public RHIGraphicCommandList
{
public:
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    void Reset() override { mCommandList->Reset(mCommandAllocator, nullptr); }
    void Clear() override { mCommandList->ClearState(nullptr); }
    void Close() override { mCommandList->Close(); }
    void SetViewPort(const std::vector<Viewport>& viewports) override
    {
        D3D12_VIEWPORT* d3d12Viewports = new D3D12_VIEWPORT[viewports.size()];
        for (uint32_t i = 0; i < viewports.size(); i++)
        {
            D3D12_VIEWPORT& vp = d3d12Viewports[i];
            vp.Width = viewports[i].mWidth;
            vp.Height = viewports[i].mHeight;
            vp.TopLeftX = viewports[i].mLeft;
            vp.TopLeftY = viewports[i].mTop;
            vp.MinDepth = viewports[i].mMinDepth;
            vp.MaxDepth = viewports[i].mMaxDepth;
        }
        mCommandList->RSSetViewports(viewports.size(), d3d12Viewports);
        delete [] d3d12Viewports;
    }
    void SetScissorRect(const std::vector<Rect>& scissorRects) override
    {
        RECT* d3d12ScissorRects = new RECT[scissorRects.size()];
        for (uint32_t i = 0; i < scissorRects.size(); i++)
        {
            RECT& rect = d3d12ScissorRects[i];
            rect.left = scissorRects[i].mLeft;
            rect.top = scissorRects[i].mTop;
            rect.right = scissorRects[i].mRight;
            rect.bottom = scissorRects[i].mBottom;
        }
        mCommandList->RSSetScissorRects(scissorRects.size(), d3d12ScissorRects);
        delete [] d3d12ScissorRects;
    }
    void CopyResource(RHIResource* dstResource, RHIResource* srcResource) override;
    
    D3D12GraphicCommandList() = default;
    D3D12GraphicCommandList(D3D12Device* parent, ID3D12CommandAllocator* pAllocator,
                     ID3D12GraphicsCommandList* pCommandList, const std::string& name) : D3D12CommandList(
        parent, pAllocator, pCommandList, name) { }
};

class D3D12CommandContext : D3D12DeviceChild, public RHICommandContext
{
public:
    D3D12Texture2D* ActiveBackBuffer() const { return mBackBuffer; }
    
    void SetName(const std::string& name) override { mName = name; }
    const std::string& GetName() const override { return mName; }
    void Reset(RHICommandQueue* pCommandQueue) override
    {
        // TODO : return all obtained command lists and cancel the trackers.
        mCommandQueue = static_cast<D3D12CommandQueue*>(pCommandQueue);
    }
    RHIGraphicCommandList* GetGraphicCommandList() override;
    RHICopyCommandList* GetCopyCommandList() override;
    RHIComputeCommandList* GetComputeCommandList() override;
    void SetRenderTarget(RHITexture2D* renderTarget, uint8_t numRenderTargets) override;
    void SetDepthStencil(RHITexture2D* depthStencil) override;
    void ExecuteCommandList(RHIGraphicCommandList* pCommandList) override;
    void ExecuteCommandList(RHICopyCommandList* pCommandList) override;
    void ExecuteCommandList(RHIComputeCommandList* pCommandList) override;
    
    D3D12CommandContext() = default;
    D3D12CommandContext(D3D12Device* parent, uint8_t numBackBuffers) : D3D12DeviceChild(parent), mNumBackBuffers(numBackBuffers), mCurrentBackBufferIndex(0) { }
    

private:
    D3D12CommandQueue* mCommandQueue;
    D3D12CommandList* mCommandList;
    D3D12Texture2D* mBackBuffer;
    D3D12FrameResource* mFrameResource;
    std::unordered_map<D3D12CommandList*, ResourceStateTracker> mStateTrackers;
};
