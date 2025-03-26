#ifdef WIN32
#include "D3dCommandList.h"

#include "Engine/render/PC/Core/RenderContext.h"
#include "Engine/common/Exception.h"

void RenderContext::setPassCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex)
{
    mPassCbvStartIdx = cbvStartIndex;
}

void RenderContext::setObjectCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex)
{
    mObjectCbvStartIdx = cbvStartIndex;
}

void RenderContext::setRenderTargets(const RenderTexture2D* renderTargets, uint8_t numRenderTargets)
{
    mRenderTargets = renderTargets;
    mNumTargetRenderTarget = numRenderTargets;
}

void RenderContext::setDepthStencilBuffer(const RenderTexture2D* depthStencilBuffer)
{
    mDepthStencilBuffer = depthStencilBuffer;
}

const RenderTexture2D* RenderContext::backBuffer() const
{
    return mRenderTargets;
}

const RenderTexture2D* RenderContext::depthStencilBuffer() const
{
    return mDepthStencilBuffer;
}

void RenderContext::setCommandQueue(ID3D12CommandQueue* pCommandQueue)
{
    mCommandQueue = pCommandQueue;
}

void RenderContext::executeCommandLists(const std::vector<D3dCommandList*>& commandLists) const
{
    uint64_t numCommandLists = commandLists.size();
    if (numCommandLists == 0) return;
    uint64_t finalNumCommandLists = 0;
    D3dCommandList* lastCommandList = commandLists[0];
    ID3D12GraphicsCommandList** cmdLists = new ID3D12GraphicsCommandList*[numCommandLists << 1];

    auto&& conversions = lastCommandList->mResourceStateTracker.buildPreTransitions();
    if (conversions.size())
    {
        D3dCommandList* preExecutions = D3dCommandListPool::getCommandList(); // D3dCommandListPool::getCommandList(type);
        preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
        preExecutions->close();
        cmdLists[0] = preExecutions->nativePtr();
        finalNumCommandLists++;
        D3dCommandListPool::recycle(preExecutions);
    }
    cmdLists[finalNumCommandLists] = lastCommandList->nativePtr();
    finalNumCommandLists++;
    D3dCommandListPool::recycle(lastCommandList);
    
    for (auto commandList : commandLists)
    {
        conversions = std::move(commandList->mResourceStateTracker.rightJoin(lastCommandList->mResourceStateTracker));
        if (conversions.size())
        {
            D3dCommandList* preExecutions = D3dCommandListPool::getCommandList();
            preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
            preExecutions->close();
            cmdLists[finalNumCommandLists] = preExecutions->nativePtr();
            finalNumCommandLists++;
            D3dCommandListPool::recycle(preExecutions);
        }
        cmdLists[finalNumCommandLists] = lastCommandList->nativePtr();
        finalNumCommandLists++;
        D3dCommandListPool::recycle(lastCommandList);
        
        lastCommandList = commandList;
    }

    mCommandQueue->ExecuteCommandLists(finalNumCommandLists, CommandListCast(cmdLists));
    delete[] cmdLists;
}

void RenderContext::executeCommandList(D3dCommandList*& commandList) const
{
    ID3D12GraphicsCommandList* cmdLists[2]; 
    auto&& conversions = commandList->mResourceStateTracker.buildPreTransitions();
    if (conversions.size())
    {
        D3dCommandList* preExecutions = D3dCommandListPool::getCommandList(); // D3dCommandListPool::getCommandList(type);
        preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
        preExecutions->close();
        cmdLists[0] = preExecutions->nativePtr();
        D3dCommandListPool::recycle(preExecutions);
    }
    cmdLists[1] = commandList->nativePtr();
    mCommandQueue->ExecuteCommandLists(2, CommandListCast(cmdLists));
}

RenderContext::RenderContext() = default;

void RenderContext::reset(ID3D12CommandQueue* pCommandQueue)
{
    mCommandQueue = pCommandQueue;
}

RenderContext::~RenderContext()
{
    reset(nullptr);
}
#endif
