#include "D3dCommandListPool.h"
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

void RenderContext::setRenderTargets(const StaticHeap* renderTargets, uint8_t numRenderTargets)
{
    mRenderTargets = renderTargets;
    mNumTargetRenderTarget = numRenderTargets;
}

void RenderContext::setDepthStencilBuffer(const StaticHeap* depthStencilBuffer)
{
    mDepthStencilBuffer = depthStencilBuffer;
}

const StaticHeap* RenderContext::backBuffer() const
{
    return mRenderTargets;
}

const StaticHeap* RenderContext::depthStencilBuffer() const
{
    return mDepthStencilBuffer;
}

void RenderContext::setCommandQueue(ID3D12CommandQueue* pCommandQueue)
{
    mCommandQueue = pCommandQueue;
}

void RenderContext::executeCommandLists(const std::vector<D3D12CommandList*>& commandLists) const
{
    uint64_t numCommandLists = commandLists.size();
    if (numCommandLists == 0) return;
    D3dCommandListType type = static_cast<D3dCommandListType>(commandLists[0]->nativePtr()->GetType());
    uint64_t finalNumCommandLists = 0;
    D3D12CommandList* lastCommandList = commandLists[0];
    ID3D12GraphicsCommandList** cmdLists = new ID3D12GraphicsCommandList*[numCommandLists << 1];
    std::vector<D3D12CommandList*> preExecutionLists{};
    preExecutionLists.reserve(numCommandLists);
    
    auto&& conversions = lastCommandList->mResourceStateTracker.BuildPreTransitions();
    if (conversions.size())
    {
        D3D12CommandList* preExecutions = D3dCommandListPool::getCommandList(type); // D3dCommandListPool::ObtainCommandList(mType);
        preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
        preExecutions->close();
        cmdLists[0] = preExecutions->nativePtr();
        finalNumCommandLists++;
        preExecutionLists.push_back(preExecutions);
    }
    cmdLists[finalNumCommandLists] = lastCommandList->nativePtr();
    finalNumCommandLists++;
    
    for (auto commandList : commandLists)
    {
        conversions = std::move(commandList->mResourceStateTracker.rightJoin(lastCommandList->mResourceStateTracker));
        if (conversions.size())
        {
            D3D12CommandList* preExecutions = D3dCommandListPool::getCommandList(type);
            preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
            preExecutions->close();
            cmdLists[finalNumCommandLists] = preExecutions->nativePtr();
            finalNumCommandLists++;
            preExecutionLists.push_back(preExecutions);
        }
        cmdLists[finalNumCommandLists] = lastCommandList->nativePtr();
        finalNumCommandLists++;

        lastCommandList = commandList;
    }

    mCommandQueue->ExecuteCommandLists(finalNumCommandLists, CommandListCast(cmdLists));
    for (auto commandList : commandLists)
    {
        commandList->mResourceStateTracker.StopTracking(type == D3dCommandListType::COPY);
    }
    for (auto preExecutionList : preExecutionLists)
    {
        D3dCommandListPool::recycle(preExecutionList);
    }
    delete[] cmdLists;
}

// close the command list before calling this func
void RenderContext::executeCommandList(D3D12CommandList* commandList) const
{
    if (!commandList) return;
    D3dCommandListType type = static_cast<D3dCommandListType>(commandList->nativePtr()->GetType());
    auto&& conversions = commandList->mResourceStateTracker.BuildPreTransitions();
    if (!conversions.size())
    {
        ID3D12GraphicsCommandList* p = commandList->nativePtr();
        mCommandQueue->ExecuteCommandLists(1, CommandListCast(&p));
        commandList->mResourceStateTracker.StopTracking(type == D3dCommandListType::COPY);
        return;
    }
    D3D12CommandList* preExecutions = D3dCommandListPool::getCommandList(type);
    preExecutions->nativePtr()->ResourceBarrier(conversions.size(), conversions.data());
    preExecutions->close();
    ID3D12GraphicsCommandList* cmdLists[2] = {preExecutions->nativePtr(), commandList->nativePtr()}; 
    mCommandQueue->ExecuteCommandLists(2, CommandListCast(cmdLists));
    commandList->mResourceStateTracker.StopTracking(type == D3dCommandListType::COPY);
    D3dCommandListPool::recycle(preExecutions);
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
