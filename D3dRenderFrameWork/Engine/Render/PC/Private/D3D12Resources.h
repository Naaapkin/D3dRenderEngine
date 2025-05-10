#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/Render/RHIDefination.h"
#include "Engine/Render/PC/Native/D3D12ConstantBufferAllocator.h"
#include "Engine/Render/PC/Native/D3D12Resource.h"

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

class D3D12Texture : public RHINativeTexture, D3D12Resource
{
    friend class D3D12SwapChain;
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
    const D3D12Resource* GetPoolResource() const;
    D3D12PooledBuffer();
    explicit D3D12PooledBuffer(const D3D12Resource* pPool, const Allocation& allocation, uint32_t stride);

private:
    const D3D12Resource* mPool;
};

class D3D12UploadBuffer : public D3D12PooledBuffer
{
public:
    uint32_t BufferSize() const override;
    void Update(const void* pData, uint64_t offset, uint64_t size) const;
    D3D12_CONSTANT_BUFFER_VIEW_DESC GetConstantDescView() const;
    void Release() override;
    D3D12UploadBuffer();
    explicit D3D12UploadBuffer(const D3D12Resource* pPool, const Allocation& allocation, uint32_t stride = sizeof(float[4]));
};

class D3D12FrameBuffer final : public D3D12Texture
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle() const { return mDescriptorHandle; }
    D3D12FrameBuffer() = default;
    D3D12FrameBuffer(const RHITextureDesc& desc, UComPtr<ID3D12Resource> pResource, const D3D12_CPU_DESCRIPTOR_HANDLE handle) : D3D12Texture(std::move(pResource), desc), mDescriptorHandle(handle) { }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE mDescriptorHandle;
};
#endif