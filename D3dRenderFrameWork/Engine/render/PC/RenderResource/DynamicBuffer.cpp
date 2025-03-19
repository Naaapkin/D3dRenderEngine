#ifdef WIN32
#include <Engine/common/Exception.h>
#include <Engine/render/PC/D3dRenderer.h>
#include <Engine/render/PC/Core/D3dGraphicContext.h>
#include <Engine/render/PC/RenderResource/DynamicBuffer.h>

void DynamicBuffer::updateRange(const byte* data, uint64_t startPos, uint64_t width) const
{
#ifdef DEBUG | _DEBUG
	ASSERT(NativePtr(), L"try to update uninitialized upload buffer");
#endif
	memcpy(mBufferMapper + startPos, data, width);
}

uint8_t* DynamicBuffer::mappedPointer() const
{
#ifdef DEBUG | _DEBUG
	ASSERT(NativePtr(), TEXT("accessing uninitialized upload buffer\n"));
#endif
	return mBufferMapper;
}

void DynamicBuffer::release()
{
	CD3DX12_RANGE readRange(0, 0);
	NativePtr()->Unmap(0, &readRange);
	D3dResource::release();
}

DynamicBuffer::~DynamicBuffer()
{
	DynamicBuffer::release();
}

DynamicBuffer::DynamicBuffer(D3dResource&& resource) : D3dResource(std::move(resource))
{
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(NativePtr()->Map(0, &readRange, reinterpret_cast<void**>(&mBufferMapper)));
}

DynamicBuffer gCreateDynamicBuffer(uint64_t size)
{
	return DynamicBuffer(::gCreateD3dResource(::gGraphicContext()->DeviceHandle(), 
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ));
}

#endif
