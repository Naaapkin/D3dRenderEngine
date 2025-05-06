#pragma once
#include "Engine/pch.h"
#include "Engine/common/helper.h"

template<class T>
class DynamicBlockAllocator : NonCopyable {
public:
    class Handle
    {
    public:
        T* Get();
        uint64_t GetVirtualAddress() const;
        Handle();
        explicit Handle(uint64_t offset, DynamicBlockAllocator* allocator);

        T& operator*() const;
        T* operator->() const;
        bool operator==(const Handle& other) const;
        bool operator!=(const Handle& other) const;
        explicit operator bool() const;
        operator uint64_t() const;

    private:
        uint64_t mBlockIndex;
        DynamicBlockAllocator* mAllocator = nullptr;
    };
    
    void Initialize(uint64_t capacity = 64, uint64_t growBlockSize = 32);
    Handle Allocate();
    void Free(Handle& handle);
    T* Resolve(uint64_t offset) const;
    void Reset();
    DynamicBlockAllocator();

    static constexpr uint32_t BLOCK_SIZE = sizeof(T);

private:
    void Grow();
    
    uint64_t mCapacity;
    uint64_t mGrowBlockSize;
    std::unique_ptr<T[]> mPool;
    uint64_t* mFreeBlocks;
    uint64_t mFreeBlocksSize;
};

template <typename T>
DynamicBlockAllocator<T>::Handle::Handle() : mBlockIndex(MAXUINT64), mAllocator(nullptr) {}

template <typename T>
DynamicBlockAllocator<T>::Handle::Handle(uint64_t offset, DynamicBlockAllocator<T>* allocator): mBlockIndex(offset), mAllocator(allocator)
{}

template <typename T>
T* DynamicBlockAllocator<T>::Handle::Get()
{
    return (mAllocator || mBlockIndex == MAXUINT64) ? mAllocator->Resolve(mBlockIndex) : nullptr;
}

template <class T>
uint64_t DynamicBlockAllocator<T>::Handle::GetVirtualAddress() const
{
    return mBlockIndex;
}

template <typename T>
T& DynamicBlockAllocator<T>::Handle::operator*() const
{
    return *mAllocator->Resolve(mBlockIndex);
}

template <typename T>
T* DynamicBlockAllocator<T>::Handle::operator->() const
{
    return mAllocator->Resolve(mBlockIndex);
}

template <typename T>
bool DynamicBlockAllocator<T>::Handle::operator==(const Handle& other) const
{
    return mBlockIndex == other.mBlockIndex && mAllocator == other.mAllocator;
}

template <typename T>
bool DynamicBlockAllocator<T>::Handle::operator!=(const Handle& other) const
{
    return !(*this == other);
}

template <typename T>
DynamicBlockAllocator<T>::Handle::operator bool() const
{
    return mAllocator != nullptr && mBlockIndex != 0;
}

template <typename T>
DynamicBlockAllocator<T>::Handle::operator uint64_t() const
{
    return mBlockIndex;
}

template <typename T>
DynamicBlockAllocator<T>::DynamicBlockAllocator() = default;

template <typename T>
void DynamicBlockAllocator<T>::Grow()
{
    uint64_t newCapacity = mCapacity + mGrowBlockSize;
    std::unique_ptr<T[]> newPool = std::make_unique<T[]>(newCapacity);
    uint64_t* newFreeBlocks = new uint64_t[newCapacity];
    if (mPool)
    {
        std::copy(mPool.get(), mPool.get() + mCapacity, newPool.get());
    }
    
    for (uint64_t i = 0; i < mGrowBlockSize; ++i)
    {
        newFreeBlocks[mFreeBlocksSize + i] = mCapacity + mGrowBlockSize - i;
    }
    
    if (mFreeBlocks)
    {
        std::copy_n(mFreeBlocks, mFreeBlocksSize, newFreeBlocks);
    }
    
    mPool.swap(newPool);
    std::swap(mFreeBlocks, newFreeBlocks);
    mFreeBlocksSize += mGrowBlockSize;
    mCapacity = newCapacity;
    
    delete[] newFreeBlocks;
}

template <typename T>
void DynamicBlockAllocator<T>::Initialize(uint64_t capacity, uint64_t growBlockSize)
{
    mCapacity = capacity;
    mGrowBlockSize = growBlockSize;
    mPool = std::make_unique<T[]>(mCapacity);
    mFreeBlocks = new uint64_t[mCapacity];
    mFreeBlocksSize = mCapacity;
        
    // Initialize Free blocks stack
    for (uint64_t i = 0; i < mCapacity; ++i)
    {
        mFreeBlocks[i] = mCapacity - i - 1;
    }
}

template <typename T>
typename DynamicBlockAllocator<T>::template Handle<T> DynamicBlockAllocator<T>::Allocate()
{
    // For simplicity, this implementation only supports single element allocation
    // Could be extended to support contiguous blocks
    // if (numElements != 1) {
    //     return Handle();
    // }

    if (mFreeBlocksSize == 0)
    {
        Grow();
    }
        
    uint64_t blockIndex = mFreeBlocks[--mFreeBlocksSize];
    return Handle(blockIndex, this);
}

template <class T>
void DynamicBlockAllocator<T>::Free(Handle& handle)
{
    uint64_t blockIndex = handle.GetVirtualAddress();
    if (blockIndex > mCapacity) {
        return;
    }
    T* ptr = (mPool.get() + blockIndex); 
    *ptr.~T();
    memset(ptr, 0, sizeof(T));
    mFreeBlocks[mFreeBlocksSize++] = blockIndex;
}

template <typename T>
T* DynamicBlockAllocator<T>::Resolve(uint64_t offset) const
{
    if (offset > mCapacity) {
        return nullptr;
    }
    return &mPool[offset]; // Blocks are 1-based
}

template <typename T>
void DynamicBlockAllocator<T>::Reset()
{
    mFreeBlocksSize = mCapacity;
    for (uint64_t i = 0; i < mCapacity; ++i) {
        mFreeBlocks[i] = mCapacity - i;
    }
}
