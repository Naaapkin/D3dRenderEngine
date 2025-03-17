#ifdef WIN32
#include <Engine/render/PC/RenderResource/DefaultHeap.h>
#include <Engine/render/Texture.h>
#include <Engine/render/PC/D3dGraphicContext.h>
#include <Engine/render/PC/D3dRenderer.h>

// void DefaultHeap::UpdateBufferResource(
// 	ID3D12GraphicsCommandList* cmdList,
// 	const void* subresourceData)
// {
// 	if (!mStagingBuffer)
// 	{
// 		mStagingBuffer = ::AllocBuffer(
// 		   D3dRenderer::GraphicContext()->DeviceHandle(),
// 		   mBuffer->GetDesc().Width,
// 		   D3D12_HEAP_TYPE_UPLOAD);
// 	}
//
// 	D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
// 		D3D12_RESOURCE_STATE_GENERIC_READ,
// 		D3D12_RESOURCE_STATE_COPY_DEST);
// 	cmdList->ResourceBarrier(1, &barrier);
//
// 	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mStagingBuffer.Get(),
// 		D3D12_RESOURCE_STATE_COMMON,
// 		D3D12_RESOURCE_STATE_GENERIC_READ);
//
// 	cmdList->ResourceBarrier(1, &barrier);
//
// 	auto Desc = mBuffer->GetDesc();
//
// 	uint8_t* pData;
// 	HRESULT hr = mStagingBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
// 	if (FAILED(hr))
// 	{
// #ifdef DEBUG | _DEBUG
// 		RY_WARN(L"无法更新默认堆: 映射上传堆失败\n");
// 		RY_WARN(WFunc::GetHRInfo(hr));
// #endif
// 		return;
// 	}
//
// 	memcpy(pData, subresourceData, Desc.Width);
// 	mStagingBuffer->Unmap(0, nullptr);
//
// 	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mStagingBuffer.Get(),
// 		D3D12_RESOURCE_STATE_GENERIC_READ,
// 		D3D12_RESOURCE_STATE_COPY_SOURCE);
//
// 	cmdList->ResourceBarrier(1, &barrier);
//
// 	cmdList->CopyResource(mBuffer.Get(), mStagingBuffer.Get());
//
// 	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mBuffer.Get(),
// 		D3D12_RESOURCE_STATE_COPY_DEST,
// 		D3D12_RESOURCE_STATE_GENERIC_READ);
// 	cmdList->ResourceBarrier(1, &barrier);
//
// 	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mStagingBuffer.Get(),
// 		D3D12_RESOURCE_STATE_COPY_SOURCE,
// 		D3D12_RESOURCE_STATE_COMMON
// 		);
// 	cmdList->ResourceBarrier(1, &barrier);
// }

DefaultHeap::DefaultHeap(D3dResource&& resource) : D3dResource(std::move(resource)) { }

// D3dResource(::GraphicContext()->DeviceHandle(),
	// 	D3D12_HEAP_FLAG_NONE,
	// 	CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	// 	CD3DX12_RESOURCE_DESC::Buffer(size),
	// 	D3D12_RESOURCE_STATE_COMMON) { }

DefaultHeap::~DefaultHeap() = default;

D3dResource* DefaultHeap::operator->()
{
	return this;
}

DefaultHeap CreateDefaultBuffer(uint64_t size)
{
	return ::CreateD3dResource(::gGraphicContext()->DeviceHandle(), 
		D3D12_HEAP_FLAG_NONE,
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON);
}
#endif
