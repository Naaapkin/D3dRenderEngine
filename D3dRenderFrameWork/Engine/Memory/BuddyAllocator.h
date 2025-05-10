#pragma once
#include "Engine/pch.h"
#include "Engine/common/helper.h"

class BuddyAllocator
{
public:
    struct Node
    {
        Node();
        Node(uint64_t offset, uint64_t size);

        uint64_t mOffset;
        uint64_t mSize;
        std::unique_ptr<Node> mLeft;
        std::unique_ptr<Node> mRight;
        bool mIsFree;
    };

    void Initialize(uint64_t totalSize = 4ull * 1024 * 1024, uint64_t blockSize = 64);
    
    uint64_t GetSize() const { return mTotalSize; }
    uint64_t GetBlockSize() const { return mBlockSize; }

    // 分配接口：返回 mOffset（内存块起始偏移）
    uint64_t Allocate(uint64_t size) const;
    void ForceFree(uint64_t offset) const;
    bool Free(uint64_t offset) const;
    BuddyAllocator();

private:
    static Node* AllocateRecursive(Node* node, uint64_t targetSize);
    static bool ForceFreeRecursive(Node* node, uint64_t offset);
    static bool FreeRecursive(Node* node, uint64_t offset);

    uint64_t mTotalSize;
    uint64_t mBlockSize;
    std::unique_ptr<Node> mRoot;
    uint8_t mMaxDepth;
};

inline BuddyAllocator::Node::Node() = default;

inline BuddyAllocator::Node::Node(uint64_t offset, uint64_t size): mOffset(offset), mSize(size), mIsFree(true)
{}

inline void BuddyAllocator::Initialize(uint64_t totalSize, uint64_t blockSize)
{
    mBlockSize = ::AlignUpToMul<uint64_t, sizeof(int)>()(blockSize);  //   align with the length of a word.
    mTotalSize = RoundUpToPowerOfTwo(std::max(::AlignUpToMul<uint64_t, sizeof(int)>()(totalSize), blockSize));
    mMaxDepth = std::_Floor_of_log_2(mTotalSize) - std::_Floor_of_log_2(mBlockSize);
    mRoot = std::make_unique<Node>(0, mTotalSize);
}

inline uint64_t BuddyAllocator::Allocate(uint64_t size) const
{
    size = RoundUpToPowerOfTwo(std::max(size, mBlockSize));
    Node* node = AllocateRecursive(mRoot.get(), size);
    if (node)
    {
        node->mIsFree = false;
        return node->mOffset;
    }
    return MAXUINT64;
}

inline void BuddyAllocator::ForceFree(uint64_t offset) const
{
    ForceFreeRecursive(mRoot.get(), offset);
}

inline bool BuddyAllocator::Free(uint64_t offset) const
{
    return FreeRecursive(mRoot.get(), offset);
}

inline BuddyAllocator::BuddyAllocator() = default;

inline BuddyAllocator::Node* BuddyAllocator::AllocateRecursive(Node* node, uint64_t targetSize)
{
    if (!node->mIsFree || node->mSize < targetSize) return nullptr;

    if (!node->mLeft)
    {
        if (node->mSize == targetSize) return node;
        uint64_t halfSize = node->mSize >> 1;
        node->mLeft = std::make_unique<Node>(node->mOffset, halfSize);
        node->mRight = std::make_unique<Node>(node->mOffset + halfSize, halfSize);
    }

    // Try allocating from children
    Node* alloc = AllocateRecursive(node->mLeft.get(), targetSize);
    return alloc ? alloc : AllocateRecursive(node->mRight.get(), targetSize);
}

inline bool BuddyAllocator::ForceFreeRecursive(Node* node, uint64_t offset)
{
    if (!node || node->mIsFree) return false;
    if (node->mLeft)
    {
        // Try children
        const bool isFreed = ForceFreeRecursive(node->mLeft.get(), offset) ||
            ForceFreeRecursive(node->mRight.get(), offset);

        // Try to merge buddies
        if (isFreed && node->mLeft->mIsFree && node->mRight->mIsFree)
        {
            node->mLeft.reset();
            node->mRight.reset();
            node->mIsFree = true;
        }

        return isFreed;
    }
        
    if (node->mOffset == offset)
    {
        node->mIsFree = true;
        return true;
    }
    return false;
}

inline bool BuddyAllocator::FreeRecursive(Node* node, uint64_t offset)
{
    if ((node->mLeft != nullptr) ^ (node->mOffset != offset)) return false;
    if (node->mOffset != offset)
    {
        return FreeRecursive(node->mLeft.get(), offset) || FreeRecursive(node->mRight.get(), offset);
    }
    node->mIsFree = true;
    return true;
}
