// ReSharper disable CppClangTidyBugproneBranchClone
#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/Render/PC/D3dUtil.h"
#include "D3D12Device.h"

class D3D12CommandObjectPool : NonCopyable
{
public:
    void Initialize(D3D12Device* device, uint32_t commandListCap = 8, uint32_t commandAllocatorCap = 8);

    ID3D12GraphicsCommandList* ObtainCommandList(D3D12_COMMAND_LIST_TYPE type);
    ID3D12CommandAllocator* ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
    //ID3D12CommandQueue* ObtainQueue(D3D12_COMMAND_LIST_TYPE type);
    void ReleaseCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList* pCommandList);
    void ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pAllocator);
    //void ReleaseQueue(ID3D12CommandQueue* pQueue);
    
    D3D12CommandObjectPool();

private:
    void AppendCommandList(D3D12_COMMAND_LIST_TYPE type, uint64_t num);
    void AppendAllocator(D3D12_COMMAND_LIST_TYPE type, uint64_t num);
    //void AppendQueues(D3D12_COMMAND_LIST_TYPE type, uint64_t size);
    
    std::stack<ID3D12GraphicsCommandList*>& GetCommandListPool(D3D12_COMMAND_LIST_TYPE type);
    std::stack<ID3D12CommandAllocator*>& GetAllocatorPool(D3D12_COMMAND_LIST_TYPE type);

    // configuration
    uint8_t mCommandListCapacity;
    uint8_t mCommandAllocatorCapacity;
    
    D3D12Device* mDevice;
    
    // storage and life manage
    std::vector<UComPtr<ID3D12GraphicsCommandList>> mCommandListContainer;
    std::vector<UComPtr<ID3D12CommandAllocator>> mAllocatorContainer;
    //std::vector<UComPtr<ID3D12CommandQueue>> mQueues;
    
    // pools
    std::stack<ID3D12GraphicsCommandList*> mDirectCommandListPool;
    std::stack<ID3D12CommandAllocator*> mDirectAllocatorPool;
    std::stack<ID3D12GraphicsCommandList*> mCopyCommandListPool;
    std::stack<ID3D12CommandAllocator*> mCopyAllocatorPool;
    std::stack<ID3D12GraphicsCommandList*> mComputeCommandListPool;
    std::stack<ID3D12CommandAllocator*> mComputeAllocatorPool;
    
    //std::stack<ID3D12CommandQueue*> mCopyQueuePool;
    //std::stack<ID3D12CommandQueue*> mComputeQueuePool;
    
    std::mutex mMutex;
};

inline void D3D12CommandObjectPool::Initialize(D3D12Device* device, uint32_t commandListCap, uint32_t commandAllocatorCap)
{
    mDevice = device;
    mCommandListCapacity = commandListCap;
    mCommandAllocatorCapacity = commandAllocatorCap;

    // 初始化命令列表/分配器池
    AppendAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocatorCapacity);
    AppendCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandListCapacity);
    AppendAllocator(D3D12_COMMAND_LIST_TYPE_COPY, mCommandAllocatorCapacity);
    AppendCommandList(D3D12_COMMAND_LIST_TYPE_COPY, mCommandListCapacity);
    AppendAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, mCommandAllocatorCapacity);
    AppendCommandList(D3D12_COMMAND_LIST_TYPE_COMPUTE, mCommandListCapacity);

    // 初始化其他队列
    //AppendQueues(D3D12_COMMAND_LIST_TYPE_COPY, 2);
    //AppendQueues(D3D12_COMMAND_LIST_TYPE_COMPUTE, 2);
}

//inline void D3D12CommandObjectPool::AppendQueues(D3D12_COMMAND_LIST_TYPE type, uint64_t size)
//{
//    D3D12_COMMAND_QUEUE_DESC desc = {};
//    desc.Type = type;
//    
//    std::lock_guard<std::mutex> lock(mMutex);
//    for (uint64_t i = 0; i < size; ++i) {
//        UComPtr<ID3D12CommandQueue> queue = mDevice->CreateCommandQueue(desc);
//        
//        if (type == D3D12_COMMAND_LIST_TYPE_COPY) {
//            mCopyQueuePool.push(queue.Get());
//        }
//        else
//        {
//            mComputeQueuePool.push(queue.Get());
//        }
//        mQueues.emplace_back(std::move(queue));
//    }
//}
//
//inline ID3D12CommandQueue* D3D12CommandObjectPool::ObtainQueue(D3D12_COMMAND_LIST_TYPE type)
//{
//    std::lock_guard<std::mutex> lock(mMutex);
//    
//    switch (type) {
//    case D3D12_COMMAND_LIST_TYPE_COPY:
//        if (mCopyQueuePool.empty()) {
//            return nullptr;
//        }
//        {
//            ID3D12CommandQueue* queue = mCopyQueuePool.top();
//            mCopyQueuePool.pop();
//            return queue;
//        }
//    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
//        if (mComputeQueuePool.empty()) {
//            return nullptr;
//        }
//        {
//            ID3D12CommandQueue* queue = mComputeQueuePool.top();
//            mComputeQueuePool.pop();
//            return queue;
//        }
//    case D3D12_COMMAND_LIST_TYPE_DIRECT:
//        THROW_EXCEPTION(TEXT("cant acquire direct command queue type"));
//    default:
//        THROW_EXCEPTION(TEXT("Unknown command queue type"));
//    }
//}
//
//inline void D3D12CommandObjectPool::ReleaseQueue(ID3D12CommandQueue* pQueue)
//{
//    if (!pQueue) return;
//    
//    std::lock_guard<std::mutex> lock(mMutex);
//    switch (pQueue->GetDesc().Type) {
//    case D3D12_COMMAND_LIST_TYPE_COPY:
//        mCopyQueuePool.push(pQueue);
//        break;
//    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
//        mComputeQueuePool.push(pQueue);
//        break;
//    default:
//        // Direct队列和Bundle队列不回收
//        break;
//    }
//}

inline D3D12CommandObjectPool::D3D12CommandObjectPool() = default;

// Helper functions to get the correct pool based on type
inline std::stack<ID3D12GraphicsCommandList*>& D3D12CommandObjectPool::GetCommandListPool(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return mDirectCommandListPool;
        case D3D12_COMMAND_LIST_TYPE_COPY: return mCopyCommandListPool;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return mComputeCommandListPool;
        default: THROW_EXCEPTION(TEXT("Unknown command list type"))
    }
}

inline std::stack<ID3D12CommandAllocator*>& D3D12CommandObjectPool::GetAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
{
    switch (type) {
        case D3D12_COMMAND_LIST_TYPE_DIRECT: return mDirectAllocatorPool;
        case D3D12_COMMAND_LIST_TYPE_COPY: return mCopyAllocatorPool;
        case D3D12_COMMAND_LIST_TYPE_COMPUTE: return mComputeAllocatorPool;
        default: THROW_EXCEPTION(TEXT("Unknown command list type"))
    }
}

inline ID3D12GraphicsCommandList* D3D12CommandObjectPool::ObtainCommandList(D3D12_COMMAND_LIST_TYPE type)
{
    std::unique_lock<std::mutex> lock(mMutex);
    auto& pool = GetCommandListPool(type);

    if (pool.empty()) {
        return nullptr;
    }

    auto pList = pool.top();
    pool.pop();
    return pList;
}

inline ID3D12CommandAllocator* D3D12CommandObjectPool::ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
    std::unique_lock<std::mutex> lock(mMutex);
    auto& pool = GetAllocatorPool(type);

    if (pool.empty()) {
        return nullptr;
    }

    auto pAllocator = pool.top();
    pool.pop();
    return pAllocator;
}

inline void D3D12CommandObjectPool::ReleaseCommandList(D3D12_COMMAND_LIST_TYPE type,
    ID3D12GraphicsCommandList* pCommandList)
{
    if (!pCommandList) return;

    std::unique_lock<std::mutex> lock(mMutex);
    GetCommandListPool(type).push(pCommandList);
}

inline void D3D12CommandObjectPool::ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* pAllocator)
{
    if (!pAllocator) return;

    // FlushCommandList the allocator before returning to pool
    pAllocator->Reset();

    std::unique_lock<std::mutex> lock(mMutex);
    GetAllocatorPool(type).push(pAllocator);
}

inline void D3D12CommandObjectPool::AppendCommandList(D3D12_COMMAND_LIST_TYPE type, uint64_t num)
{
    std::vector<UComPtr<ID3D12GraphicsCommandList>> newLists;
    newLists.reserve(num);

    ID3D12CommandAllocator* pAllocator = ObtainCommandAllocator(type);
    for (uint64_t i = 0; i < num; ++i) {
        UComPtr<ID3D12GraphicsCommandList> pList = mDevice->CreateCommandList(type, pAllocator);
        pList->Close(); // Command lists are created in recording state
        newLists.push_back(std::move(pList));
    }
    ReleaseCommandAllocator(type, pAllocator);

    std::unique_lock<std::mutex> lock(mMutex);
    auto& pool = GetCommandListPool(type);
    for (auto& list : newLists) {
        pool.push(list.Get());
        mCommandListContainer.push_back(std::move(list));
    }
}

inline void D3D12CommandObjectPool::AppendAllocator(D3D12_COMMAND_LIST_TYPE type, uint64_t num)
{
    std::vector<UComPtr<ID3D12CommandAllocator>> newAllocators;
    newAllocators.reserve(num);

    for (uint64_t i = 0; i < num; ++i) {
        UComPtr<ID3D12CommandAllocator> pAllocator = mDevice->CreateCommandAllocator(type);
        newAllocators.push_back(std::move(pAllocator));
    }

    std::unique_lock<std::mutex> lock(mMutex);
    std::stack<ID3D12CommandAllocator*>& pool = GetAllocatorPool(type);
    for (auto& allocator : newAllocators) {
        pool.push(allocator.Get());
        mAllocatorContainer.push_back(std::move(allocator));
    }
}
#endif