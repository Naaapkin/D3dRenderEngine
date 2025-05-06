// ReSharper disable CppClangTidyBugproneBranchClone
#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/D3dUtil.h"
#include "D3D12Device.h"

class D3D12CommandObjectPool : Singleton<D3D12CommandObjectPool>
{
public:
    // 初始化方法
    void Initialize(D3D12Device* device,
                    uint64_t queueBlockSize = 4,
                    uint64_t listAllocatorBlockSize = 8);

    ID3D12GraphicsCommandList* ObtainCommandList(D3D12_COMMAND_LIST_TYPE type);
    ID3D12CommandAllocator* ObtainCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
    ID3D12CommandQueue* ObtainQueue(D3D12_COMMAND_LIST_TYPE type);
    void ReleaseCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList* pCommandList);
    void ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pAllocator);
    void ReleaseQueue(ID3D12CommandQueue* pQueue);
    
    D3D12CommandObjectPool();

private:
    // 内部实现方法
    void AppendCommandList(D3D12_COMMAND_LIST_TYPE type, uint64_t num);
    void AppendAllocator(D3D12_COMMAND_LIST_TYPE type, uint64_t num);
    void AppendQueues(D3D12_COMMAND_LIST_TYPE type, uint64_t size);

    // 辅助函数
    std::stack<ID3D12GraphicsCommandList*>& GetCommandListPool(D3D12_COMMAND_LIST_TYPE type);
    std::stack<ID3D12CommandAllocator*>& GetAllocatorPool(D3D12_COMMAND_LIST_TYPE type);

    // 配置参数
    uint8_t mListAllocatorBlockSize;
    uint8_t mQueueBlockSize;

    // 核心数据成员
    D3D12Device* mDevice;
    
    // 命令列表/分配器存储
    std::vector<UComPtr<ID3D12GraphicsCommandList>> mCommandListContainer;
    std::vector<UComPtr<ID3D12CommandAllocator>> mAllocatorContainer;
    UComPtr<ID3D12CommandQueue> mDirectQueue;
    std::vector<UComPtr<ID3D12CommandQueue>> mQueues;
    
    // 对象池
    std::stack<ID3D12GraphicsCommandList*> mDirectCommandListPool;
    std::stack<ID3D12CommandAllocator*> mDirectAllocatorPool;
    std::stack<ID3D12GraphicsCommandList*> mCopyCommandListPool;
    std::stack<ID3D12CommandAllocator*> mCopyAllocatorPool;
    std::stack<ID3D12GraphicsCommandList*> mComputeCommandListPool;
    std::stack<ID3D12CommandAllocator*> mComputeAllocatorPool;
    std::stack<ID3D12CommandQueue*> mCopyQueuePool;
    std::stack<ID3D12CommandQueue*> mComputeQueuePool;

    // 线程安全
    std::mutex mMutex;
};

inline void D3D12CommandObjectPool::Initialize(D3D12Device* device,
                                               uint64_t queueBlockSize,
                                               uint64_t listAllocatorBlockSize)
{
    mDevice = device;
    mQueueBlockSize = queueBlockSize;
    mListAllocatorBlockSize = listAllocatorBlockSize;

    // 初始化默认direct队列
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    mDirectQueue = mDevice->CreateCommandQueue(desc);

    // 初始化命令列表/分配器池
    AppendCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, mListAllocatorBlockSize);
    AppendAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, mListAllocatorBlockSize);

    // 初始化其他队列
    AppendQueues(D3D12_COMMAND_LIST_TYPE_COPY, mQueueBlockSize);
    AppendQueues(D3D12_COMMAND_LIST_TYPE_COMPUTE, mQueueBlockSize);
}

inline void D3D12CommandObjectPool::AppendQueues(D3D12_COMMAND_LIST_TYPE type, uint64_t size)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    
    std::lock_guard<std::mutex> lock(mMutex);
    for (uint64_t i = 0; i < size; ++i) {
        UComPtr<ID3D12CommandQueue> queue = mDevice->CreateCommandQueue(desc);
        
        if (type == D3D12_COMMAND_LIST_TYPE_COPY) {
            mCopyQueuePool.push(queue.Get());
        }
        else
        {
            mComputeQueuePool.push(queue.Get());
        }
        mQueues.emplace_back(std::move(queue));
    }
}

inline ID3D12CommandQueue* D3D12CommandObjectPool::ObtainQueue(D3D12_COMMAND_LIST_TYPE type)
{
    std::lock_guard<std::mutex> lock(mMutex);
    
    switch (type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return mDirectQueue.Get();
        
    case D3D12_COMMAND_LIST_TYPE_COPY:
        if (mCopyQueuePool.empty()) {
            AppendQueues(type, mQueueBlockSize);
        }
        {
            ID3D12CommandQueue* queue = mCopyQueuePool.top();
            mCopyQueuePool.pop();
            return queue;
        }
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        if (mComputeQueuePool.empty()) {
            AppendQueues(type, mQueueBlockSize);
        }
        {
            ID3D12CommandQueue* queue = mComputeQueuePool.top();
            mComputeQueuePool.pop();
            return queue;
        }
    default:
        throw std::runtime_error("Unknown command queue type");
    }
}

inline void D3D12CommandObjectPool::ReleaseQueue(ID3D12CommandQueue* pQueue)
{
    if (!pQueue) return;
    
    std::lock_guard<std::mutex> lock(mMutex);
    switch (pQueue->GetDesc().Type) {
    case D3D12_COMMAND_LIST_TYPE_COPY:
        mCopyQueuePool.push(pQueue);
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        mComputeQueuePool.push(pQueue);
        break;
    default:
        // Direct队列和Bundle队列不回收
        break;
    }
}

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
        lock.unlock(); // Unlock while creating new objects
        AppendCommandList(type, mListAllocatorBlockSize);
        lock.lock(); // Relock to check again
    }

    if (pool.empty()) {
        THROW_EXCEPTION(TEXT("No command list available"))
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
        lock.unlock(); // Unlock while creating new objects
        AppendAllocator(type, mListAllocatorBlockSize);
        lock.lock(); // Relock to check again
    }

    if (pool.empty()) {
        THROW_EXCEPTION(TEXT("No command list available"))
    }

    auto pAllocator = pool.top();
    pool.pop();
    return pAllocator;
}

inline void D3D12CommandObjectPool::ReleaseCommandList(D3D12_COMMAND_LIST_TYPE type,
    ID3D12GraphicsCommandList* pCommandList)
{
    if (!pCommandList) return;

    // Reset the command list before returning to pool
    pCommandList->Reset(nullptr, nullptr);

    std::unique_lock<std::mutex> lock(mMutex);
    GetCommandListPool(type).push(pCommandList);
}

inline void D3D12CommandObjectPool::ReleaseCommandAllocator(D3D12_COMMAND_LIST_TYPE type,
    ID3D12CommandAllocator* pAllocator)
{
    if (!pAllocator) return;

    // Reset the allocator before returning to pool
    pAllocator->Reset();

    std::unique_lock<std::mutex> lock(mMutex);
    GetAllocatorPool(type).push(pAllocator);
}

inline void D3D12CommandObjectPool::AppendCommandList(D3D12_COMMAND_LIST_TYPE type, uint64_t num)
{
    std::vector<UComPtr<ID3D12GraphicsCommandList>> newLists;
    newLists.reserve(num);

    for (uint64_t i = 0; i < num; ++i) {
        UComPtr<ID3D12GraphicsCommandList> pList = mDevice->CreateCommandList(type, ObtainCommandAllocator(type));
        pList->Close(); // Command lists are created in recording state
        newLists.push_back(std::move(pList));
    }

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
    auto& pool = GetAllocatorPool(type);
    for (auto& allocator : newAllocators) {
        pool.push(allocator.Get());
        mAllocatorContainer.push_back(std::move(allocator));
    }
}
#endif