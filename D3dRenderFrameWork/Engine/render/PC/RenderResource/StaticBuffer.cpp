#ifdef WIN32
#include <Engine/render/PC/RenderResource/StaticBuffer.h>
#include <Engine/render/PC/Core/D3dGraphicContext.h>
#include <Engine/render/PC/D3dRenderer.h>

StaticBuffer::StaticBuffer(D3dResource&& resource) : D3dResource(std::move(resource)) { }

StaticBuffer::~StaticBuffer() = default;

StaticBuffer gCreateStaticBuffer(uint64_t size)
{
	return ::gCreateD3dResource(::gGraphicContext()->DeviceHandle(), 
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON);
}
#endif
