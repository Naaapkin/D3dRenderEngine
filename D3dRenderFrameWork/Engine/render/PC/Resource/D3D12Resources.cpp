#ifdef WIN32
#include "D3D12Resources.h"
#include "Engine/render/PC/Native/D3D12EnumConversions.h"

uint32_t D3D12Buffer::BufferSize() const
{ return mDesc.mSize; }

const D3D12Resource* D3D12Buffer::GetD3D12Resource() const
{ return this; }

void D3D12Buffer::Release()
{ mResource.Release(); }

D3D12Buffer::D3D12Buffer() = default;

D3D12Buffer::D3D12Buffer(UComPtr<ID3D12Resource> pResource, const RHIBufferDesc& desc) : D3D12Resource(std::move(pResource)), RHINativeBuffer(desc)
{
}

uint32_t D3D12VertexBuffer::VertexSize() const
{ return mVertexSize; }

uint32_t D3D12VertexBuffer::VertexCount() const
{ return mVertexCount; }

D3D12_VERTEX_BUFFER_VIEW D3D12VertexBuffer::GetVertexBufferView() const
{
	return {D3D12Buffer::GetD3D12Resource()->GetGPUVirtualAddress(), mVertexCount * mVertexSize, mVertexSize };
}

D3D12VertexBuffer::D3D12VertexBuffer() = default;

D3D12VertexBuffer::D3D12VertexBuffer(UComPtr<ID3D12Resource> pResource, const RHIBufferDesc& desc,
	uint32_t vertexSize, uint32_t vertexCount): D3D12Buffer(std::move(pResource), desc),
	                                                                     mVertexSize(vertexSize), mVertexCount(vertexCount)
{
}

uint32_t D3D12IndexBuffer::IndexCount() const
{ return mIndexCount; }

Format D3D12IndexBuffer::IndexFormat() const
{ return mIndexFormat; }

D3D12_INDEX_BUFFER_VIEW D3D12IndexBuffer::GetIndexBufferView() const
{
	return {
		D3D12Buffer::GetD3D12Resource()->GetGPUVirtualAddress(), BufferSize(),
		mIndexFormat == Format::R16_UINT ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
	};
}

D3D12IndexBuffer::D3D12IndexBuffer() = default;

D3D12IndexBuffer::D3D12IndexBuffer(UComPtr<ID3D12Resource>&& pResource, const RHIBufferDesc& desc, 
	uint32_t indexCount, Format indexFormat) : D3D12Buffer(std::move(pResource), desc),
																		mIndexCount(indexCount), mIndexFormat(indexFormat)
{
}

D3D12Texture::D3D12Texture() = default;

D3D12Texture::D3D12Texture(UComPtr<ID3D12Resource> pResource, const RHITextureDesc& desc) : RHINativeTexture(desc), D3D12Resource(std::move(pResource)) { }

const D3D12Resource* D3D12Texture::GetD3D12Resource() const
{
    return this;
}

D3D12_SHADER_RESOURCE_VIEW_DESC D3D12Texture::GetSRVDesc() const
{
    return ::ConvertToD3D12SRVDesc(GetDesc());
}

void D3D12Texture::Release()
{ mResource.Release(); }

D3D12_GPU_VIRTUAL_ADDRESS D3D12PooledBuffer::GpuAddress() const
{
	return mGPUAddress;
}

void D3D12PooledBuffer::SetGpuAddress(const D3D12_GPU_VIRTUAL_ADDRESS gpuAddress)
{
	mGPUAddress = gpuAddress;
}

void* D3D12PooledBuffer::CpuAddress() const
{
	return mCPUAddress;
}

void D3D12PooledBuffer::SetCpuAddress(void* const cpuAddress)
{
	mCPUAddress = cpuAddress;
}

uint64_t D3D12PooledBuffer::Offset() const
{
	return mOffset;
}

void D3D12PooledBuffer::SetOffset(const uint64_t offset)
{
	mOffset = offset;
}

uint64_t D3D12PooledBuffer::Size() const
{
	return mSize;
}

void D3D12PooledBuffer::SetSize(const uint64_t size)
{
	mSize = size;
}

D3D12PooledBuffer::D3D12PooledBuffer(): Allocation(MAXUINT64, nullptr, 0, 0)
{ }

D3D12PooledBuffer::D3D12PooledBuffer(const Allocation& allocation): Allocation(allocation), RHINativeBuffer({ Size(), ResourceType::DYNAMIC })
{
}

uint32_t D3D12ConstantBuffer::BufferSize() const
{
	return static_cast<uint32_t>(Size());
}

void D3D12ConstantBuffer::Update(const void* pData, uint64_t offset, uint64_t size) const
{
	memcpy(static_cast<byte*>(CpuAddress()) + offset, pData, std::min(size, Size()));
}

D3D12_CONSTANT_BUFFER_VIEW_DESC D3D12ConstantBuffer::GetConstantDescView() const
{
	return { GpuAddress(), static_cast<uint32_t>(Size()) };
}

D3D12ConstantBuffer::D3D12ConstantBuffer() = default;

D3D12ConstantBuffer::D3D12ConstantBuffer(const Allocation& allocation): D3D12PooledBuffer(allocation)
{
}

void D3D12ConstantBuffer::Release()
{
}

uint32_t D3D12StagingBuffer::BufferSize() const
{
	return static_cast<uint32_t>(Size());
}

void D3D12StagingBuffer::Update(const void* pData, uint64_t offset, uint64_t size) const
{
	memcpy(static_cast<byte*>(CpuAddress()) + offset, pData, std::min(size, Size()));
}

void D3D12StagingBuffer::Release()
{
}

D3D12StagingBuffer::D3D12StagingBuffer() = default;

D3D12StagingBuffer::D3D12StagingBuffer(const Allocation& allocation): D3D12PooledBuffer(allocation)
{
}
#endif
