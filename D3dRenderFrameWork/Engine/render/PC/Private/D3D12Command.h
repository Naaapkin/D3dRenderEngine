#pragma once
#include "Engine/pch.h" 
#include "Engine/render/RHICommand.h"

class D3D12Command : public RHICommand
{
public:
    D3D12Command() = default;
    virtual void Execute(ID3D12GraphicsCommandList* pCommandList) = 0;
    virtual ~D3D12Command() = default;
};

class D3D12CommandResourceBarrier : public D3D12Command
{
public:
    D3D12CommandResourceBarrier() = default;

    void SetBarriers(D3D12_RESOURCE_BARRIER* barriers, uint32_t numBarriers)
    {
        mBarriers.assign(barriers, barriers + numBarriers);
    }

    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->ResourceBarrier(mBarriers.size(), mBarriers.data());
    }
    
private:
    std::vector<D3D12_RESOURCE_BARRIER> mBarriers;
};

class D3D12CommandCopyResource : public D3D12Command
{
public:
    void SetCopyParameters(ID3D12Resource* pSource, ID3D12Resource* pDestination)
    {
        mSource = pSource;
        mDestination = pDestination;
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->CopyResource(mDestination, mSource);
    }

    D3D12CommandCopyResource() = default;
    ~D3D12CommandCopyResource() override = default;
    
private:
    ID3D12Resource* mSource;
    ID3D12Resource* mDestination;
};

class D3D12CommandCopyBufferRegion : public D3D12Command
{
public:
    void SetCopyParameters(ID3D12Resource* pSource, ID3D12Resource* pDestination, uint64_t size, uint64_t sourceOffset = 0,
        uint64_t destinationOffset = 0)
    {
        mSource = pSource;
        mDestination = pDestination;
        mSourceOffset = sourceOffset;
        mDestinationOffset = destinationOffset;
        mSize = size;
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->CopyBufferRegion(mDestination, mDestinationOffset, mSource, mSourceOffset, mSize);
    }

    D3D12CommandCopyBufferRegion() = default;
    ~D3D12CommandCopyBufferRegion() override = default;

private:
    ID3D12Resource* mSource;
    ID3D12Resource* mDestination;
    uint64_t mSourceOffset;
    uint64_t mDestinationOffset;
    uint64_t mSize;
};

class D3D12CommandCopyTextureRegion : public D3D12Command
{
public:
    void SetCopyParameters(ID3D12Resource* pSource, ID3D12Resource* pDestination, const D3D12_BOX& sourceBox,
        const D3D12_TEXTURE_COPY_LOCATION& sourceLocation,
        const D3D12_TEXTURE_COPY_LOCATION& destinationLocation,
        uint32_t dstX, uint32_t dstY, uint32_t dstZ)
    {
        mSource = pSource;
        mDestination = pDestination;
        mSourceLocation = sourceLocation;
        mDestinationLocation = destinationLocation;
        mSourceBox = sourceBox;
        mDstX = dstX;
        mDstY = dstY;
        mDstZ = dstZ;
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->CopyTextureRegion(
            &mDestinationLocation, mSourceBox.left, mSourceBox.top, mSourceBox.front,
            &mSourceLocation, &mSourceBox);
    }

    D3D12CommandCopyTextureRegion() = default;
    ~D3D12CommandCopyTextureRegion() override = default;

private:
    ID3D12Resource* mSource;
    ID3D12Resource* mDestination;
    D3D12_BOX mSourceBox;
    uint32_t mDstX;
    uint32_t mDstY;
    uint32_t mDstZ;
    D3D12_TEXTURE_COPY_LOCATION mSourceLocation;
    D3D12_TEXTURE_COPY_LOCATION mDestinationLocation;
};

class D3D12CommandSetRTDS : public D3D12Command
{
public:
    D3D12CommandSetRTDS() = default;

    void SetRenderTargets(const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, uint32_t numRTVs)
    {
        mRTVs.assign(rtvs, rtvs + numRTVs);
    }

    void SetDepthStencilTarget(const D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
    {
        mDSV = *dsv;
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->OMSetRenderTargets(mRTVs.size(), mRTVs.data(), 0, &mDSV);
    }

private:
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> mRTVs;
    D3D12_CPU_DESCRIPTOR_HANDLE mDSV;
};

class D3D12CommandClearRenderTarget : public D3D12Command
{
    using Color = float[4];
public:
    void SetClearDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const Color color , const RECT* pRects, const uint32_t numRects)
    {
        mRTV = rtv;
        memcpy(mColor, color, sizeof(Color));
        mClearRects.reset(new RECT[numRects]);
        mNumRects = numRects;
        memcpy(mClearRects.get(), pRects, sizeof(RECT) * mNumRects);
    }

    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->ClearRenderTargetView(mRTV, mColor, mNumRects, mClearRects.get());
    }
    
    D3D12CommandClearRenderTarget() = default;
    
private:
    D3D12_CPU_DESCRIPTOR_HANDLE mRTV;
    Color mColor;
    uint32_t mNumRects;
    std::unique_ptr<RECT[]> mClearRects;
};

class D3D12CommandClearDepthStencil : public D3D12Command
{
public:
    void SetClearDescriptors(D3D12_CPU_DESCRIPTOR_HANDLE dsv, D3D12_CLEAR_FLAGS clearFlags, const RECT* clearRects, uint32_t numClearRects, float depth = 1.0f, uint32_t stencil = 0)
    {
        mDSV = dsv;
        mClearFlags = clearFlags;
        mClearRects.reset(new RECT[numClearRects]);
        mNumRects = numClearRects;
        mDepth = depth;
        mStencil = stencil;
        memcpy(mClearRects.get(), clearRects, sizeof(RECT) * numClearRects);
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->ClearDepthStencilView(mDSV, mClearFlags, 1.0f, 0, mNumRects, mClearRects.get());
    }

    D3D12CommandClearDepthStencil() = default;
    
private:
    D3D12_CPU_DESCRIPTOR_HANDLE mDSV;
    D3D12_CLEAR_FLAGS mClearFlags;
    std::unique_ptr<RECT[]> mClearRects;
    uint32_t mNumRects;
    float mDepth;
    uint32_t mStencil;
};

class D3D12CommandSetViewports : public D3D12Command
{
public:
    void SetViewport(const D3D12_VIEWPORT* viewport, uint32_t numViewports)
    {
        mViewports.assign(viewport, viewport + numViewports);
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->RSSetViewports(mViewports.size(), mViewports.data());
    }

private:
    std::vector<D3D12_VIEWPORT> mViewports;
};

class D3D12CommandSetScissors : public D3D12Command
{
public:
    void SetScissorRects(const RECT* ScissorRects, uint32_t numScissorRects)
    {
        mScissorRects.assign(ScissorRects, ScissorRects + numScissorRects);
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->RSSetScissorRects(mScissorRects.size(), mScissorRects.data());
    }
    
private:
    std::vector<RECT> mScissorRects;
};

class D3D12CommandBindVertexBuffer : public D3D12Command
{
public:
    D3D12CommandBindVertexBuffer() = default;

    void SetVertexBuffers(const D3D12_VERTEX_BUFFER_VIEW* pVertexBufferView, uint32_t numVertexBuffers)
    {
        mVBVs.assign(pVertexBufferView, pVertexBufferView + numVertexBuffers);
    }

    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->IASetVertexBuffers(0, mVBVs.size(), mVBVs.data());
    }
    
private:
    std::vector<D3D12_VERTEX_BUFFER_VIEW> mVBVs;
};

class D3D12CommandBindIndexBuffer : public D3D12Command
{
public:
    D3D12CommandBindIndexBuffer() = default;

    void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* pIndexBuffer)
    {
        mIBV = *pIndexBuffer;
    }

    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->IASetIndexBuffer(&mIBV);
    }

private:
    D3D12_INDEX_BUFFER_VIEW mIBV;
};

class D3D12CommandDrawIndexedInstanced : public D3D12Command
{
public:
    D3D12CommandDrawIndexedInstanced() = default;

    void SetPipelineState(ID3D12PipelineState* pPipelineState)
    {
        mPipelineState = pPipelineState;
    }

    void SetSubMesh(uint32_t indexPerInstance, uint32_t startIndexLocation, uint32_t instanceCount)
    {
        mIndexPerInstance = indexPerInstance;
        mStartIndexLocation = startIndexLocation;
        mInstanceCount = instanceCount;
    }
    
    void Execute(ID3D12GraphicsCommandList* pCommandList) override
    {
        pCommandList->SetPipelineState(mPipelineState);
        pCommandList->DrawIndexedInstanced(mIndexPerInstance, mInstanceCount, mStartIndexLocation, 0, 0);
    }

private:
    uint32_t mIndexPerInstance;
    uint32_t mInstanceCount;
    uint32_t mStartIndexLocation;
    ID3D12PipelineState* mPipelineState;
};