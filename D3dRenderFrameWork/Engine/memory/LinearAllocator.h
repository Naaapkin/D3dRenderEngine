#pragma once
#include "Engine/common/helper.h"
#include "Engine/pch.h"

class LinearAllocator : NonCopyable
{
public:
    void Initialize(uint64_t size = 1024ull);
    uint64_t Allocate(uint64_t size = 1);
    void Reset();
    LinearAllocator();
    
private:
    uint64_t mTotalSize;
    uint64_t mOccupiedSize;
};

inline void LinearAllocator::Initialize(uint64_t size)
{
    mTotalSize = size;
    mOccupiedSize = size;
}

inline uint64_t LinearAllocator::Allocate(uint64_t size)
{
    if (mOccupiedSize + size >= mTotalSize) return MAXUINT64;
    mOccupiedSize += size;
    return mOccupiedSize;
}

inline void LinearAllocator::Reset()
{
    mOccupiedSize = 0;
}

inline LinearAllocator::LinearAllocator() = default;
