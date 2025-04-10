#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Core/DescriptorHeap.h"
#include "Engine/render/PC/Resource/D3dAllocator.h"

class Fence;
enum class Format : uint8_t;

class D3dContext
{
public:
    ID3D12Device* deviceHandle() const;
    IDXGIFactory4* factoryHandle() const;
    Fence createFence(uint64_t initValue) const;
    ID3D12CommandQueue* createCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
    ID3D12CommandAllocator* createCommandAllocator(D3D12_COMMAND_LIST_TYPE type) const;
    ID3D12GraphicsCommandList* createCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12CommandAllocator* pAllocator) const;
    IDXGISwapChain1* createSwapChainForHwnd(ID3D12CommandQueue* pCommandQueue, Format backBufferFormat, uint8_t numBackBuffer) const;
    DescriptorHeap* createDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint16_t size) const;
    ID3D12PipelineState* createPipelineStateObject(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) const;
    ID3D12RootSignature* createRootSignature(ID3DBlob* binary) const;

    static D3dContext& instance();
    void initialize();

    D3dContext();
    ~D3dContext();
    
    DELETE_COPY_OPERATOR(D3dContext)
    DELETE_COPY_CONSTRUCTOR(D3dContext);
    DELETE_MOVE_OPERATOR(D3dContext);
    DELETE_MOVE_CONSTRUCTOR(D3dContext);

private:    
#if defined(DEBUG) or defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> mInfoQueue;
#endif
    ComPtr<IDXGIFactory4> mFactoryHandle;
    // IDXGIAdapter** mAdapters = nullptr;
    ComPtr<ID3D12Device> mDeviceHandle;
};

#endif
