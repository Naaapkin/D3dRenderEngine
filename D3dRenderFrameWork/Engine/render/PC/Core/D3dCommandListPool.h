#pragma once
#include "Engine/pch.h"

class D3dRenderer;
class D3dCommandList;
class D3dContext;

enum class D3dCommandListType : uint8_t
{
    DIRECT = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    COMPUTE = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,
    COPY = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
};

struct TypedCommandListPool
{
    static constexpr float EXPAND_RATIO = 0.2f;
    
    D3dCommandListType mType;
    
    D3dCommandList** mCommandLists;
    uint64_t mNumAllocators;
    uint64_t mNumAvailableAllocators;
    uint64_t mNumCommandLists;
    uint64_t mNumAvailableCmdLists;
    std::unordered_map<std::thread::id, ID3D12CommandAllocator**> mAllocators;

    void initialize(const D3dContext& d3dContext, D3dCommandListType type, uint64_t numCommandList, uint8_t numCpuWorkPages);
    void appendCommandList(const D3dContext& d3dContext, uint64_t num);
    D3dCommandList* getCommandList(const D3dContext& d3dContext);
    void recycleCommandList(D3dCommandList* pCommandList);
    ID3D12CommandAllocator* getCommandAllocator();
    TypedCommandListPool(D3dCommandListType type = D3dCommandListType::DIRECT);
};

class D3dCommandListPool
{
public:
    static void initialize(D3dContext& gc, uint8_t numCpuWorkPages);
    static void recycle(D3dCommandList* pCommandList);
    static D3dCommandList* getCommandList(D3dCommandListType type);
    static ID3D12CommandAllocator* sGetCommandAllocator(D3dCommandListType type);

private:
    static D3dCommandListPool& instance();
    
    ID3D12CommandAllocator* getCommandAllocator(D3dCommandListType type) const;

    D3dCommandListPool();

    static constexpr uint16_t GRAPHIC_ALLOCATOR_CAPACITY = 4;
    static constexpr uint16_t GRAPHIC_COMMAND_LIST_CAPACITY = 8;
    static constexpr uint16_t COMPUTE_ALLOCATOR_CAPACITY = 16;
    static constexpr uint16_t COMPUTE_COMMAND_LIST_CAPACITY = 32;
    static constexpr uint16_t COPY_ALLOCATOR_CAPACITY = 8;
    static constexpr uint16_t COPY_COMMAND_LIST_CAPACITY = 16;
    
    D3dContext* mGraphicContext;
    TypedCommandListPool* mCommandListPools;
};
