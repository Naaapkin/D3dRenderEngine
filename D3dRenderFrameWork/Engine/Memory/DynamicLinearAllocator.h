#pragma once
#include "Engine/pch.h"
#include "Engine/common/Exception.h"
#include "Engine/common/helper.h"

class DynamicLinearAllocator : NonCopyable
{
public:
    template <typename T>
    class Handle
    {
    public:
        Handle();
        explicit Handle(uint64_t offset, DynamicLinearAllocator* allocator);
        T* Get() const;
        template<typename TParent, typename = std::enable_if_t<std::is_base_of_v<TParent, T>>>
        Handle<TParent> As();

        T& operator*() const;
        T* operator->() const;
        bool operator==(const Handle& other) const;
        bool operator!=(const Handle& other) const;
        
        operator uint64_t() const;
        explicit operator bool() const;

    private:
        uint64_t mOffset = 0;          // 相对于分配器基址的偏移量
        DynamicLinearAllocator* mAllocator = nullptr; // 所属分配器
    };
    
    template <class TPtr, class TValue = TPtr, typename = std::enable_if_t<std::is_base_of_v<TPtr, TValue>>>
    Handle<TPtr> Allocate(uint64_t numElements = 1);
    template<typename T>
    T* Resolve(uint64_t offset) const;

    void Initialize(uint64_t initialSize = 1048576ul, uint32_t alignment = 8, uint32_t blockSize = 64 * 1024);
    void Reset();
    DynamicLinearAllocator();
    DynamicLinearAllocator(DynamicLinearAllocator&& other) noexcept;
    ~DynamicLinearAllocator();

    DynamicLinearAllocator& operator=(DynamicLinearAllocator&& other) noexcept;

private:
    void Grow();

    uint8_t* mMemoryPool = nullptr;
    uint64_t mMemoryPoolSize = 0;
    uint64_t mOccupiedSize = 0;
    
    uint32_t mAlignment = 0;
    uint32_t mBlockSize = 0;
};

template <typename T>
DynamicLinearAllocator::Handle<T>::Handle() = default;

template <typename T>
DynamicLinearAllocator::Handle<T>::Handle(uint64_t offset, DynamicLinearAllocator* allocator): mOffset(offset), mAllocator(allocator)
{}

template <typename T>
T& DynamicLinearAllocator::Handle<T>::operator*() const
{
    return *Get();
}

template <typename T>
T* DynamicLinearAllocator::Handle<T>::operator->() const
{
    return Get();
}

template <typename T>
T* DynamicLinearAllocator::Handle<T>::Get() const
{
    if (!mAllocator) return nullptr;
    return mAllocator->template Resolve<T>(mOffset);
}

template <typename T>
template <typename TParent, typename>
DynamicLinearAllocator::Handle<TParent> DynamicLinearAllocator::Handle<T>::As()
{
    return Handle<TParent>{mOffset, mAllocator};
}

template <typename T>
DynamicLinearAllocator::Handle<T>::operator bool() const
{
    return mAllocator != nullptr;
}

template <typename T>
bool DynamicLinearAllocator::Handle<T>::operator==(const Handle& other) const
{
    return mOffset == other.mOffset && mAllocator == other.mAllocator;
}

template <typename T>
bool DynamicLinearAllocator::Handle<T>::operator!=(const Handle& other) const
{
    return !(*this == other);
}

template <typename T>
DynamicLinearAllocator::Handle<T>::operator unsigned long long() const
{
    return reinterpret_cast<uint64_t>(Get());
}

template <class TPtr, class TValue, typename>
DynamicLinearAllocator::Handle<TPtr> DynamicLinearAllocator::Allocate(uint64_t numElements)
{
    constexpr uint64_t size = sizeof(TValue);
    constexpr uint64_t alignment = alignof(TValue);
        
    uint64_t alignedSize = ((size + alignment - 1) & ~(alignment - 1)) * numElements;
    if (mOccupiedSize + alignedSize > mMemoryPoolSize) Grow();
    
    uint64_t offset = mOccupiedSize;
    mOccupiedSize += alignedSize;
    return Handle<TPtr>(offset, this);
}

template <typename T>
T* DynamicLinearAllocator::Resolve(uint64_t offset) const
{
    ASSERT(offset < mOccupiedSize, TEXT("violate memory access"));
    return reinterpret_cast<T*>(mMemoryPool + offset);
}

inline void DynamicLinearAllocator::Initialize(uint64_t initialSize, uint32_t alignment, uint32_t blockSize)
{
    if (mMemoryPool) return;
        
    mAlignment = alignment;
    mBlockSize = blockSize;
    mMemoryPoolSize = initialSize;
        
    // 使用VirtualAlloc确保内存对齐
    mMemoryPool = static_cast<uint8_t*>(MEM_VIRTUAL_ALLOC(initialSize));
    if (!mMemoryPool) throw std::bad_alloc();
        
    mOccupiedSize = 0;
}

inline void DynamicLinearAllocator::Reset()
{
    mOccupiedSize = 0;
}

inline DynamicLinearAllocator::DynamicLinearAllocator() = default;

inline DynamicLinearAllocator::DynamicLinearAllocator(DynamicLinearAllocator&& other)  noexcept
{
    std::swap(mMemoryPool, other.mMemoryPool);
    std::swap(mAlignment, other.mAlignment);
    std::swap(mBlockSize, other.mBlockSize);
    std::swap(mOccupiedSize, other.mOccupiedSize);
}

inline DynamicLinearAllocator& DynamicLinearAllocator::operator=(DynamicLinearAllocator&& other) noexcept
{
    if (&other != this)
    {
        std::swap(mMemoryPool, other.mMemoryPool);
        std::swap(mAlignment, other.mAlignment);
        std::swap(mBlockSize, other.mBlockSize);
        std::swap(mOccupiedSize, other.mOccupiedSize);
    }
    return *this;
}

inline DynamicLinearAllocator::~DynamicLinearAllocator()
{
    if (mMemoryPool)
    {
        MEM_VIRTUAL_FREE(mMemoryPool);
    }
}

inline void DynamicLinearAllocator::Grow()
{
    const uint64_t newSize = mMemoryPoolSize + mBlockSize;
        
    uint8_t* newPool = static_cast<uint8_t*>(MEM_VIRTUAL_ALLOC(newSize));
    if (!newPool) throw std::bad_alloc();
        
    if (mMemoryPool)
    {
        memcpy(newPool, mMemoryPool, mOccupiedSize);
        MEM_VIRTUAL_FREE(mMemoryPool);
    }
        
    mMemoryPool = newPool;
    mMemoryPoolSize = newSize;
}
