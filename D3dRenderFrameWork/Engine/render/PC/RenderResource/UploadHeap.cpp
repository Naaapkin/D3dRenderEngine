#ifdef WIN32
#include "UploadHeap.h"
#include "render/PC/D3dRenderer.h"
#include "render/PC/D3dGraphicContext.h"
#include "common/PC/WException.h"

void UploadHeap::UpdateRange(const byte* data, uint64_t startPos, uint64_t width)
{
#ifdef DEBUG | _DEBUG
	ASSERT(NativePtr(), L"try to update uninitialized upload buffer");
#endif
	memcpy(mBufferMapper + startPos, data, width);
}

uint8_t* UploadHeap::MappedPointer() const
{
#ifdef DEBUG | _DEBUG
	if (!NativePtr())
		RY_WARN(L"accessing uninitialized upload buffer\n");
#endif
	return mBufferMapper;
}

void UploadHeap::Release()
{
	CD3DX12_RANGE readRange(0, 0);
	NativePtr()->Unmap(0, &readRange);
	D3dResource::Release();
}

UploadHeap::~UploadHeap()
{
	UploadHeap::Release();
}

D3dResource* UploadHeap::operator->()
{
	return this;
}

UploadHeap::UploadHeap(D3dResource&& resource) : D3dResource(std::move(resource))
{
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(NativePtr()->Map(0, &readRange, reinterpret_cast<void**>(&mBufferMapper)));
}

UploadHeap CreateUploadBuffer(uint64_t size)
{
	return UploadHeap(::CreateD3dResource(::GraphicContext()->DeviceHandle(), 
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ));
}

#endif
