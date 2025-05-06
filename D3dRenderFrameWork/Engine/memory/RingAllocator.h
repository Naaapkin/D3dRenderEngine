#pragma once

#include "Engine/pch.h"
#include "Engine/common/helper.h"

class UnsafeRingAllocator : NonCopyable
{
public:
    void Initialize(uint64_t size = 1024ull)
    {
        mTotalSize = size;
        mTail = 0;
    }

    uint64_t Allocate(uint64_t size)
    {
        if (size == 0 || size > mTotalSize) {
            return MAXUINT64; // 无效大小
        }

        if (mTail + size <= mTotalSize) {
            uint64_t offset = mTail;
            mTail += size;
            return offset;
        }
        uint64_t offset = 0;
        mTail = size;
        return offset;
    }

    void Reset()
    {
        mTail = 0;
    }

    uint64_t GetTotalSize() const { return mTotalSize; }
    uint64_t GetUsedSize() const { return mTail; }

    UnsafeRingAllocator() = default;

private:
    uint64_t mTotalSize; // 环形缓冲区总大小
    uint64_t mTail;      // 下一个分配位置
};

class RingAllocator : NonCopyable
{
public:
    void Initialize(uint64_t size = 1024ull)
    {
        mTotalSize = size;
        mHead = 0;
        mTail = 0;
        mOccupiedSize = 0;
    }

    // Allocate from tail
    uint64_t Allocate(uint64_t size = 1)
    {
        if (size > mTotalSize)
            return UINT64_MAX; // Fail: too large

        if (mOccupiedSize + size > mTotalSize)
            return UINT64_MAX; // Fail: not enough space

        uint64_t offset = mTail;
        mTail = (mTail + size) % mTotalSize;
        mOccupiedSize += size;
        return offset; // return the offset
    }

    // Free from head
    void Free(uint64_t size = 1)
    {
	    size = std::min(size, mOccupiedSize); // prevent underflow

        mHead = (mHead + size) % mTotalSize;
        mOccupiedSize -= size;
    }

    void Reset()
    {
        mHead = 0;
        mTail = 0;
        mOccupiedSize = 0;
    }

    uint64_t GetTotalSize() const { return mTotalSize; }
    uint64_t GetUsedSize() const { return mOccupiedSize; }
    uint64_t GetTail() const { return mTail; }
    uint64_t GetHead() const { return mHead; }

    RingAllocator() = default;

private:
    uint64_t mTotalSize = 0;
    uint64_t mOccupiedSize = 0;
    uint64_t mHead = 0;
    uint64_t mTail = 0;
};