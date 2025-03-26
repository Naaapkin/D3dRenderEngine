#ifdef WIN32
#include "Engine/render/PC/Core/D3dCommandListPool.h"
#include "Engine/render/PC/Core/D3dContext.h"
#include "Engine/render/PC/Core/D3dRenderer.h"

void D3dCommandListPool::initialize(D3dContext& gc)
{
    static auto& pool = instance();
    // guard rendering thread access.
    if (!D3dRenderer::sIsRenderingThread() && pool.mGraphicContext) return;
    pool = {&gc, COMMAND_LIST_CAPACITY, ALLOCATOR_CAPACITY * static_cast<uint64_t>(D3dRenderer::sNumCpuWorkPages())};
}

void D3dCommandListPool::recycle(D3dCommandList* commandList)
{
    static auto& pool = instance();
#if defined(DEBUG) || defined(_DEBUG)
    DEBUG_WARN("recycling unclosed command list.");
    commandList->nativePtr()->Close();
#endif
    pool.mCommandLists[pool.mNumAvailableCmdLists++] = commandList;
}

D3dCommandList* D3dCommandListPool::getCommandList()
{
    static auto& pool = instance();
    // TODO: this is still allocating allocators dynamically
    if (pool.mNumAvailableCmdLists == 0) pool.appendCommandList(static_cast<uint64_t>(static_cast<float>(pool.mNumCommandLists) * EXPAND_RATIO));
    D3dCommandList* pCommandList = pool.mCommandLists[--pool.mNumAvailableCmdLists];
    pCommandList->reset(sGetCommandAllocator());
    return pCommandList;
}

ID3D12CommandAllocator* D3dCommandListPool::sGetCommandAllocator()
{
    return instance().getCommandAllocator();
}

D3dCommandListPool& D3dCommandListPool::instance()
{
    static D3dCommandListPool instance{};
    return instance;
}

void D3dCommandListPool::appendCommandList(uint64_t num)
{
    uint64_t newSize = mNumCommandLists + num;
    D3dCommandList** commandLists = new D3dCommandList*[newSize];
    memcpy(static_cast<void*>(commandLists), static_cast<void const*>(mCommandLists), sizeof(D3dCommandList**) * mNumCommandLists);
    delete[] mCommandLists;
    ID3D12CommandAllocator* pAllocator = getCommandAllocator(); 
    for (uint64_t i = mNumCommandLists; i < newSize; ++i)
    {
        ID3D12GraphicsCommandList* pCommandList = mGraphicContext->createCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, pAllocator);
        pCommandList->Close();
        commandLists[mNumAvailableCmdLists++] = new D3dCommandList{pCommandList};
    }
    mCommandLists = commandLists;
    mNumCommandLists = newSize;
}

ID3D12CommandAllocator* D3dCommandListPool::getCommandAllocator()
{
    return mCommandAllocators[std::this_thread::get_id()][D3dRenderer::sCpuWorkingPageIdx()];
}

D3dCommandListPool::D3dCommandListPool() = default;

D3dCommandListPool::D3dCommandListPool(D3dContext* pGc, uint64_t cmdAllocatorsCapacity,
                                       uint64_t cmdListsCapacity):
    mGraphicContext(pGc), mCommandLists(nullptr),
    mNumAllocators(cmdAllocatorsCapacity),
    mNumAvailableAllocators(cmdAllocatorsCapacity), mNumCommandLists(0),
    mNumAvailableCmdLists(0)
{
    auto& allocators = mCommandAllocators[std::this_thread::get_id()];
    allocators = new ID3D12CommandAllocator*[cmdAllocatorsCapacity];
    for (int i = 0; i < cmdAllocatorsCapacity; ++i)
    {
        allocators[i] = mGraphicContext->createCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    appendCommandList(cmdListsCapacity);
}
#endif