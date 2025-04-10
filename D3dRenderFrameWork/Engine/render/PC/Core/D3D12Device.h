#pragma once

#include "Engine/pch.h"
#include "Engine/render/Shader.h"
#include "Engine/render/PC/Resource/D3D12Resources.h"
#include "Engine/render/PC/Resource/RHIDescriptors.h"

class D3D12CommandQueue;

class D3D12Device
{
public:
    ID3D12Resource* CreateCommitedResource(const D3D12_HEAP_PROPERTIES& heapProp,
        const D3D12_HEAP_FLAGS& heapFlags, const D3D12_RESOURCE_DESC& desc,
        const D3D12_CLEAR_VALUE& clearValue,
        D3D12_RESOURCE_STATES initialState) const;
    void CreatePipelineStateObject(GraphicPipelineStateDesc psoDesc);
    ID3D12DescriptorHeap* CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flag, uint32_t numDescriptors) const;
    ID3D12RootSignature* CreateRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) const;
    ID3D12CommandQueue* CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC& queueDesc) const;

    // ID3D12Heap* CreateCommitedHeap(const D3D12_HEAP_PROPERTIES& heapProp)
    D3D12Device(ID3D12Device* device = nullptr);;

    DELETE_COPY_OPERATOR(D3D12Device);
    DELETE_COPY_CONSTRUCTOR(D3D12Device);
    
private:
    static std::vector<D3D12_STATIC_SAMPLER_DESC> GetStaticSamplers();
    UComPtr<ID3D12Device> mDevice;

    std::unordered_map<Shader*, ID3D12PipelineState*> mPsoCache;
};
