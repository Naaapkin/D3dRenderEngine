#include "D3dRenderer.h"
#ifdef WIN32
#include "Engine/render/PC/Core/D3dCommandListPool.h"
#include "Engine/render/PC/Core/D3dContext.h"

void TypedCommandListPool::initialize(const D3dContext& d3dContext, D3dCommandListType type, uint64_t numCommandList, uint8_t numCpuWorkPages)
{
    this->mType = type;
    mNumAvailableAllocators = numCpuWorkPages;
    auto& allocators = mAllocators[std::this_thread::get_id()];
    allocators = new ID3D12CommandAllocator*[numCpuWorkPages];
    for (int i = 0; i < numCpuWorkPages; ++i)
    {
        allocators[i] = d3dContext.createCommandAllocator(static_cast<D3D12_COMMAND_LIST_TYPE>(type));
    }
    appendCommandList(d3dContext, numCommandList);
}

void TypedCommandListPool::appendCommandList(const D3dContext& d3dContext, uint64_t num)
{
    uint64_t newSize = mNumCommandLists + num;
    D3dCommandList** commandLists = new D3dCommandList*[newSize];
    memcpy(static_cast<void*>(commandLists), static_cast<void const*>(mCommandLists), sizeof(D3dCommandList**) * mNumCommandLists);
    delete[] mCommandLists;
    ID3D12CommandAllocator* pAllocator = mAllocators[std::this_thread::get_id()][D3dRenderer::sCpuWorkingPageIdx()]; 
    for (uint64_t i = mNumCommandLists; i < newSize; ++i)
    {
        ID3D12GraphicsCommandList* pCommandList = d3dContext.createCommandList(static_cast<D3D12_COMMAND_LIST_TYPE>(mType), pAllocator);
        pCommandList->Close();
        commandLists[mNumAvailableCmdLists++] = new D3dCommandList{pCommandList};
    }
    mCommandLists = commandLists;
    mNumCommandLists = newSize;
}

D3dCommandList* TypedCommandListPool::getCommandList(const D3dContext& d3dContext)
{
    // TODO: this is still allocating allocators dynamically
    if (mNumAvailableCmdLists == 0) appendCommandList(d3dContext, static_cast<uint64_t>(static_cast<float>(mNumCommandLists) * EXPAND_RATIO));
    D3dCommandList* pCommandList = mCommandLists[--mNumAvailableCmdLists];
    pCommandList->reset(getCommandAllocator());
    return pCommandList;
}

void TypedCommandListPool::recycleCommandList(D3dCommandList* pCommandList)
{
    mCommandLists[mNumAvailableCmdLists++] = pCommandList;
}

ID3D12CommandAllocator* TypedCommandListPool::getCommandAllocator()
{
    return mAllocators[std::this_thread::get_id()][D3dRenderer::sCpuWorkingPageIdx()];
}

TypedCommandListPool::TypedCommandListPool(D3dCommandListType type) : mType(type), mCommandLists(nullptr), mNumAllocators(0), mNumAvailableAllocators(0), mNumCommandLists(0), mNumAvailableCmdLists(0) { }

void D3dCommandListPool::initialize(D3dContext& gc, uint8_t numCpuWorkPages)
{
    auto& pool = instance();
    // guard rendering thread access.
    if (!D3dRenderer::sIsRenderingThread() && pool.mGraphicContext) return;
    pool.mGraphicContext = &gc;
    pool.mCommandListPools = new TypedCommandListPool[3];
    pool.mCommandListPools[0].initialize(gc, D3dCommandListType::DIRECT, GRAPHIC_COMMAND_LIST_CAPACITY, numCpuWorkPages);
    pool.mCommandListPools[1].initialize(gc, D3dCommandListType::COMPUTE, COMPUTE_COMMAND_LIST_CAPACITY, numCpuWorkPages);
    pool.mCommandListPools[2].initialize(gc, D3dCommandListType::COPY, COPY_COMMAND_LIST_CAPACITY, numCpuWorkPages);
}

void D3dCommandListPool::recycle(D3dCommandList* pCommandList)
{
    if (!pCommandList) return;
    auto& pool = instance();
    switch (static_cast<D3dCommandListType>(pCommandList->nativePtr()->GetType()))
    {
    case D3dCommandListType::DIRECT:
        return pool.mCommandListPools[0].recycleCommandList(pCommandList);
    case D3dCommandListType::COMPUTE:
        return pool.mCommandListPools[1].recycleCommandList(pCommandList);
    case D3dCommandListType::COPY:
        return pool.mCommandListPools[2].recycleCommandList(pCommandList);
    }
}

D3dCommandList* D3dCommandListPool::getCommandList(D3dCommandListType type)
{
    auto& pool = instance();
    // TODO: this is still allocating allocators dynamically
    switch (type)
    {
        case D3dCommandListType::DIRECT:
            return pool.mCommandListPools[0].getCommandList(*pool.mGraphicContext);
        case D3dCommandListType::COMPUTE:
            return pool.mCommandListPools[1].getCommandList(*pool.mGraphicContext);
        case D3dCommandListType::COPY:
            return pool.mCommandListPools[2].getCommandList(*pool.mGraphicContext);
    }
    return nullptr;
}

ID3D12CommandAllocator* D3dCommandListPool::sGetCommandAllocator(D3dCommandListType type)
{
    auto& pool = instance();
    switch (type)
    {
    case D3dCommandListType::DIRECT:
        return pool.mCommandListPools[0].getCommandAllocator();
    case D3dCommandListType::COMPUTE:
        return pool.mCommandListPools[1].getCommandAllocator();
    case D3dCommandListType::COPY:
        return pool.mCommandListPools[2].getCommandAllocator();
    }
    return nullptr;
}

D3dCommandListPool& D3dCommandListPool::instance()
{
    static D3dCommandListPool instance{};
    return instance;
}

ID3D12CommandAllocator* D3dCommandListPool::getCommandAllocator(D3dCommandListType type) const
{
    return mCommandListPools[static_cast<uint8_t>(type)].mAllocators[std::this_thread::get_id()][
        D3dRenderer::sCpuWorkingPageIdx()];
}

D3dCommandListPool::D3dCommandListPool() = default;
#endif