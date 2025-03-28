#pragma once
#include "Engine/common/Exception.h"
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"

class DynamicBuffer : public D3dResource
{
public:
	void updateRange(const byte* data, uint64_t startPos, uint64_t width) const;
	byte* mappedPointer() const;
	uint64_t size() const;
	void release() override;
	
	DynamicBuffer();
	DynamicBuffer(D3dResource&& resource);
	DynamicBuffer(DynamicBuffer&& other) noexcept;
	~DynamicBuffer() override;
	;
	DynamicBuffer& operator=(DynamicBuffer&& other) noexcept;
	bool operator==(const DynamicBuffer&& other) const noexcept;
	bool operator!=(const DynamicBuffer& other) const noexcept;
	DELETE_COPY_CONSTRUCTOR(DynamicBuffer)
	DELETE_COPY_OPERATOR(DynamicBuffer)
	
private:
	byte* mBufferMapper;
};

inline DynamicBuffer::DynamicBuffer(DynamicBuffer&& other) noexcept : D3dResource(std::move(other)), mBufferMapper(other.mBufferMapper)
{
	other.mBufferMapper = nullptr;
}

inline void DynamicBuffer::updateRange(const byte* data, uint64_t startPos, uint64_t width) const
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(nativePtr(), TEXT("try to update uninitialized upload buffer\n"));
#endif
	memcpy(mBufferMapper + startPos, data, width);
}

inline uint8_t* DynamicBuffer::mappedPointer() const
{
#if defined(DEBUG) or defined(_DEBUG)
	ASSERT(nativePtr(), TEXT("accessing uninitialized upload buffer\n"));
#endif
	return mBufferMapper;
}

inline uint64_t DynamicBuffer::size() const
{
	return nativePtr()->GetDesc().Width;
}

inline void DynamicBuffer::release()
{
	CD3DX12_RANGE readRange(0, 0);
	nativePtr()->Unmap(0, &readRange);
	D3dResource::release();
}

inline DynamicBuffer::DynamicBuffer() = default;

inline DynamicBuffer::~DynamicBuffer()
{
	DynamicBuffer::release();
}

inline DynamicBuffer& DynamicBuffer::operator=(DynamicBuffer&& other) noexcept
{
	D3dResource::operator=(std::move( other ));
	if (D3dObject::operator!=(other))
	{
		mBufferMapper = other.mBufferMapper;

		other.mBufferMapper = nullptr;
	}
	return *this;
}

inline bool DynamicBuffer::operator==(const DynamicBuffer&& other) const noexcept
{
	return D3dObject::operator==(other);
}

inline bool DynamicBuffer::operator!=(const DynamicBuffer& other) const noexcept
{
	return D3dObject::operator!=(other);
}

inline DynamicBuffer::DynamicBuffer(D3dResource&& resource) : D3dResource(std::move(resource))
{
	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(DynamicBuffer::nativePtr()->Map(0, &readRange, reinterpret_cast<void**>(&mBufferMapper)));
}
#endif
