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

    uint64_t AllocateAligned(uint64_t size, uint64_t alignment)
    {
        ASSERT((alignment & (alignment - 1)) == 0, TEXT("alignment must be power of two"));
        if (size == 0 || alignment == 0) {
            return MAXUINT64; // 非法参数
        }

        // 计算当前 tail 对齐后的 offset
        uint64_t alignedOffset = (mTail + alignment - 1) & ~(alignment - 1);

        // 如果对齐后 + size 超出总大小，则从头开始
        if (alignedOffset + size > mTotalSize) {
            alignedOffset = 0;
            mTail = alignedOffset + size;
            return alignedOffset;
        }

        // 正常分配
        mTail = alignedOffset + size;
        return alignedOffset;
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

    // AllocateAligned from tail
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

    uint64_t AllocateAligned(uint64_t size, uint64_t alignment)
    {
        ASSERT((alignment & (alignment - 1)) == 0, TEXT("alignment must be power of two"));
        if (size > mTotalSize || alignment == 0)
            return UINT64_MAX; // Fail: invalid size or alignment not power of two

        // Try to align from current tail
        uint64_t alignedTail = (mTail + alignment - 1) & ~(alignment - 1);
        uint64_t padding = alignedTail - mTail;
        uint64_t totalRequired = padding + size;

        if (totalRequired > mTotalSize)
            return UINT64_MAX; // Fail: impossible even with wrap

        // Case 1: enough space from tail to end (no wrap)
        if (mTail + totalRequired <= mTotalSize) {
            if (mOccupiedSize + totalRequired > mTotalSize)
                return UINT64_MAX; // Fail: not enough room
            uint64_t offset = alignedTail;
            mTail = (mTail + totalRequired) % mTotalSize;
            mOccupiedSize += totalRequired;
            return offset;
        }

        // Case 2: wrap-around: align from beginning
        alignedTail = 0;
        padding = alignedTail; // usually 0
        totalRequired = padding + size;

        if (mOccupiedSize + totalRequired > mTotalSize)
            return UINT64_MAX; // Fail: not enough space after wrap

        uint64_t offset = alignedTail;
        mTail = totalRequired; // after wrap
        mOccupiedSize += totalRequired;
        return offset;
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