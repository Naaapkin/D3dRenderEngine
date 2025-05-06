#pragma once
#include "Engine/common/helper.h"
#include "Engine/pch.h"

class BlockAllocator : NonCopyable
{
public:
    void Initialize(uint64_t size = 1024ull)
    {
        mTotalSize = size;
        mFreeBlocks.reset(new uint64_t[size]);
        mBlockStates.reset(new bool[size]);
        for (uint64_t i = 0; i < size; i++)
        {
            mFreeBlocks[i] = size - i - 1;
        }
        mNumFreeBlocks = size;
    }
    uint64_t Allocate()
    {
        if (!mNumFreeBlocks) return MAXUINT64;
        return mFreeBlocks[--mNumFreeBlocks];
    }
    std::unique_ptr<uint64_t[]> Allocate(uint64_t size)
    {
        if (mNumFreeBlocks < size) return nullptr;
        uint64_t* offsets = new uint64_t[size];
        for (uint64_t i = 0; i < size; i++)
        {
            offsets[i] = mFreeBlocks[--mNumFreeBlocks];
        }
        return std::unique_ptr<uint64_t[]>(offsets);
    }
    bool Free(uint64_t offset)
    {
        if (!mBlockStates[offset]) return false;
        mBlockStates[offset] = false;
        mFreeBlocks[mNumFreeBlocks++] = offset;
        return true;
    }
    BlockAllocator() = default;
private:
    uint64_t mTotalSize;
    std::unique_ptr<bool[]> mBlockStates;
    std::unique_ptr<uint64_t[]> mFreeBlocks;
    uint64_t mNumFreeBlocks;
};
