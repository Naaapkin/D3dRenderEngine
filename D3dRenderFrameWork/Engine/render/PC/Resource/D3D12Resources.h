#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/RHIDefination.h"
#include "Engine/render/PC/Native/D3D12ConstantBufferAllocator.h"
#include "Engine/render/PC/Native/D3D12Resource.h"

class D3D12Buffer : D3D12Resource, public RHINativeBuffer
{
public:
    uint32_t BufferSize() const override;
    const D3D12Resource* GetD3D12Resource() const;
    void Release() override;

    D3D12Buffer();
    D3D12Buffer(UComPtr<ID3D12Resource> pResource, const RHIBufferDesc& desc);
};

class D3D12VertexBuffer final : public D3D12Buffer
{
public:
    uint32_t VertexSize() const;
    uint32_t VertexCount() const;

    D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

    D3D12VertexBuffer();
    D3D12VertexBuffer(UComPtr<ID3D12Resource> pResource, const RHIBufferDesc& desc, uint32_t vertexSize,
			uint32_t vertexCount);

private:
    uint32_t mVertexSize;
    uint32_t mVertexCount;
};

class D3D12IndexBuffer final : public D3D12Buffer
{
public:
    uint32_t IndexCount() const;
    Format IndexFormat() const;

    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const;

    D3D12IndexBuffer();
    D3D12IndexBuffer(UComPtr<ID3D12Resource>&& pResource, const RHIBufferDesc& desc, uint32_t indexCount,
        Format indexFormat);

private:
    uint32_t mIndexCount;
    Format mIndexFormat;
};

class D3D12Texture final : public RHINativeTexture, D3D12Resource
{
public:
    const D3D12Resource* GetD3D12Resource() const;
    D3D12_SHADER_RESOURCE_VIEW_DESC GetSRVDesc() const;
    void Release() override;

    D3D12Texture();
    D3D12Texture(UComPtr<ID3D12Resource> pResource, const RHITextureDesc& desc);
};

class D3D12PooledBuffer : public Allocation, public RHINativeBuffer
{
public:
    D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const;
    void SetGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS gpuAddress);
    void* CpuAddress() const;
    void SetCpuAddress(void* cpuAddress);
    uint64_t Offset() const;
    void SetOffset(uint64_t offset);
    uint64_t Size() const;
    void SetSize(uint64_t size);
    D3D12PooledBuffer();
    explicit D3D12PooledBuffer(const Allocation& allocation);
};

class D3D12ConstantBuffer : public D3D12PooledBuffer
{
public:
    uint32_t BufferSize() const override;
    void Update(const void* pData, uint64_t offset, uint64_t size) const;
    D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantDescView() const;
    void Release() override;
    D3D12ConstantBuffer();
    explicit D3D12ConstantBuffer(const Allocation& allocation);
};

class D3D12StagingBuffer : public D3D12PooledBuffer
{
public:
    uint32_t BufferSize() const override;
    void Update(const void* pData, uint64_t offset, uint64_t size) const;
    void Release() override;
    D3D12StagingBuffer();
    explicit D3D12StagingBuffer(const Allocation& allocation);
};
#endif