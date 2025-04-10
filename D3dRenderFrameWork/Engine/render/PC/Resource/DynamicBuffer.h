#pragma once
#include "Engine/common/Exception.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"

class DynamicHeap : public D3dResource
{
public:
	void updateRange(const byte* data, uint64_t startPos, uint64_t width) const;
	byte* mappedPointer() const;
	uint64_t size() const;
	void release() override;
	
	DynamicHeap();
	DynamicHeap(D3dResource&& resource);
	DynamicHeap(DynamicHeap&& other) noexcept;
	~DynamicHeap() override;
	;
	DynamicHeap& operator=(DynamicHeap&& other) noexcept;
	bool operator==(const DynamicHeap&& other) const noexcept;
	bool operator!=(const DynamicHeap& other) const noexcept;
	DELETE_COPY_CONSTRUCTOR(DynamicHeap)
	DELETE_COPY_OPERATOR(DynamicHeap)
	
private:
	byte* mBufferMapper;
};

inline DynamicHeap::DynamicHeap(DynamicHeap&& other) noexcept : D3dResource(std::move(other)), mBufferMapper(other.mBufferMapper)
{
	other.mBufferMapper = nullptr;
}

inline void DynamicHeap::updateRange(const byte* data, uint64_t startPos, uint64_t width) const
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(nativePtr(), TEXT("try to update uninitialized upload buffer\n"));
#endif
	memcpy(mBufferMapper + startPos, data, width);
}

inline uint8_t* DynamicHeap::mappedPointer() const
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(nativePtr(), TEXT("accessing uninitialized upload buffer\n"));
#endif
	return mBufferMapper;
}

inline uint64_t DynamicHeap::size() const
{
	return nativePtr()->GetDesc().Width;
}

inline void DynamicHeap::release()
{
	CD3DX12_RANGE readRange(0, 0);
	nativePtr()->Unmap(0, &readRange);
	D3dResource::release();
}

inline DynamicHeap::DynamicHeap() = default;

inline DynamicHeap::~DynamicHeap()
{
	DynamicHeap::release();
}

inline DynamicHeap& DynamicHeap::operator=(DynamicHeap&& other) noexcept
{
	D3dResource::operator=(std::move( other ));
	if (D3D12DeviceChild::operator!=(other))
	{
		mBufferMapper = other.mBufferMapper;

		other.mBufferMapper = nullptr;
	}
	return *this;
}

inline bool DynamicHeap::operator==(const DynamicHeap&& other) const noexcept
{
	return D3D12DeviceChild::operator==(other);
}

inline bool DynamicHeap::operator!=(const DynamicHeap& other) const noexcept
{
	return D3D12DeviceChild::operator!=(other);
}

inline DynamicHeap::DynamicHeap(D3dResource&& resource) : D3dResource(std::move(resource))
{
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(DynamicHeap::nativePtr()->Map(0, &readRange, reinterpret_cast<void**>(&mBufferMapper)));
}
#endif
