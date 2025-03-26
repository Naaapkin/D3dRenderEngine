#pragma once
#include "Engine/pch.h"

class D3dCommandList;
class D3dContext;

enum class D3dCommandListType : uint8_t
{
    DIRECT = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
    COPY = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY,
    COMPUTE = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE,
};

class D3dCommandListPool
{
public:
    static void initialize(D3dContext& gc);
    static void recycle(D3dCommandList* commandList);
    static D3dCommandList* getCommandList();
    static ID3D12CommandAllocator* sGetCommandAllocator(); 

private:
    static D3dCommandListPool& instance();
    void appendCommandList(uint64_t num);
    ID3D12CommandAllocator* getCommandAllocator();
    
    D3dCommandListPool();
    D3dCommandListPool(D3dContext* pGc, uint64_t cmdAllocatorsCapacity, uint64_t cmdListsCapacity);

    static constexpr uint16_t ALLOCATOR_CAPACITY = 32;
    static constexpr uint16_t COMMAND_LIST_CAPACITY = 32;
    static constexpr float EXPAND_RATIO = 0.2f;

    D3dContext* mGraphicContext;
    D3dCommandList** mCommandLists;
    uint64_t mNumAllocators;
    uint64_t mNumAvailableAllocators;
    uint64_t mNumCommandLists;
    uint64_t mNumAvailableCmdLists;
    std::unordered_map<std::thread::id, ID3D12CommandAllocator**> mCommandAllocators; 
};
