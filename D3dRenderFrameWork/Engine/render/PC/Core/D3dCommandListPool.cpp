#include "D3dRenderer.h"
#ifdef WIN32
#include "Engine/render/PC/Core/D3dCommandListPool.h"
#include "Engine/render/PC/Core/D3dContext.h"

void TypedCommandListPool::Initialize(D3dCommandListType type, uint64_t numCommandList, uint8_t numCpuWorkPages)
{
    this->mType = type;
    mNumAvailableAllocators = numCpuWorkPages;
    auto& allocators = mAllocators[std::this_thread::get_id()];
    allocators = new ID3D12CommandAllocator*[numCpuWorkPages];
    for (int i = 0; i < numCpuWorkPages; ++i)
    {
        allocators[i] = D3dContext::instance().createCommandAllocator(static_cast<D3D12_COMMAND_LIST_TYPE>(type));
    }
    AppendCommandList(numCommandList);
}

void TypedCommandListPool::AppendCommandList(uint64_t num)
{
    uint64_t newSize = mNumCommandLists + num;
    D3D12CommandList** commandLists = new D3D12CommandList*[newSize];
    memcpy(static_cast<void*>(commandLists), static_cast<void const*>(mCommandLists), sizeof(D3D12CommandList**) * mNumCommandLists);
    delete[] mCommandLists;
    ID3D12CommandAllocator* pAllocator = mAllocators[std::this_thread::get_id()][D3dRenderer::sCpuWorkingPageIdx()]; 
    for (uint64_t i = mNumCommandLists; i < newSize; ++i)
    {
        ID3D12GraphicsCommandList* pCommandList = D3dContext::instance().createCommandList(static_cast<D3D12_COMMAND_LIST_TYPE>(mType), pAllocator);
        pCommandList->Close();
        commandLists[mNumAvailableCmdLists++] = new D3D12CommandList{pCommandList};
    }
    mCommandLists = commandLists;
    mNumCommandLists = newSize;
}

D3D12CommandList* TypedCommandListPool::ObtainCommandList()
{
    // TODO: this is still allocating allocators dynamically
    if (mNumAvailableCmdLists == 0) AppendCommandList(static_cast<float>(mNumCommandLists) * EXPAND_RATIO);
    D3D12CommandList* pCommandList = mCommandLists[--mNumAvailableCmdLists];
    pCommandList->reset(ObtainCommandAllocator());
    return pCommandList;
}

void TypedCommandListPool::ReleaseCommandList(D3D12CommandList* pCommandList)
{
    mCommandLists[mNumAvailableCmdLists++] = pCommandList;
}

ID3D12CommandAllocator* TypedCommandListPool::ObtainCommandAllocator()
{
    return mAllocators[std::this_thread::get_id()][D3dRenderer::sCpuWorkingPageIdx()];
}

void D3dCommandListPool::initialize(uint8_t numCpuWorkPages)
{
    auto& pool = instance();
    // guard rendering thread access.
    if (!D3dRenderer::sIsRenderingThread()) return;
    
    pool.mCommandListPools = new TypedCommandListPool[3];
    pool.mCommandListPools[0].Initialize(D3dCommandListType::DIRECT, GRAPHIC_COMMAND_LIST_CAPACITY, numCpuWorkPages);
    pool.mCommandListPools[1].Initialize(D3dCommandListType::COMPUTE, COMPUTE_COMMAND_LIST_CAPACITY, numCpuWorkPages);
    pool.mCommandListPools[2].Initialize(D3dCommandListType::COPY, COPY_COMMAND_LIST_CAPACITY, numCpuWorkPages);
}

void D3dCommandListPool::recycle(D3D12CommandList* pCommandList)
{
    if (!pCommandList) return;
    auto& pool = instance();
    switch (static_cast<D3dCommandListType>(pCommandList->nativePtr()->GetType()))
    {
    case D3dCommandListType::DIRECT:
        return pool.mCommandListPools[0].ReleaseCommandList(pCommandList);
    case D3dCommandListType::COMPUTE:
        return pool.mCommandListPools[1].ReleaseCommandList(pCommandList);
    case D3dCommandListType::COPY:
        return pool.mCommandListPools[2].ReleaseCommandList(pCommandList);
    }
}

D3D12CommandList* D3dCommandListPool::getCommandList(D3dCommandListType type)
{
    auto& pool = instance();
    // TODO: this is still allocating allocators dynamically
    switch (type)
    {
        case D3dCommandListType::DIRECT:
            return pool.mCommandListPools[0].ObtainCommandList();
        case D3dCommandListType::COMPUTE:
            return pool.mCommandListPools[1].ObtainCommandList();
        case D3dCommandListType::COPY:
            return pool.mCommandListPools[2].ObtainCommandList();
    }
    return nullptr;
}

ID3D12CommandAllocator* D3dCommandListPool::sGetCommandAllocator(D3dCommandListType type)
{
    auto& pool = instance();
    switch (type)
    {
    case D3dCommandListType::DIRECT:
        return pool.mCommandListPools[0].ObtainCommandAllocator();
    case D3dCommandListType::COMPUTE:
        return pool.mCommandListPools[1].ObtainCommandAllocator();
    case D3dCommandListType::COPY:
        return pool.mCommandListPools[2].ObtainCommandAllocator();
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