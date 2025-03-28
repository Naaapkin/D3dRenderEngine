#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"

class StaticBuffer : public D3dResource
{
	friend class D3dAllocator;
public:
	uint64_t size() const override;
    StaticBuffer(StaticBuffer&& other) noexcept;
	~StaticBuffer() override;
	StaticBuffer& operator=(StaticBuffer&& other) noexcept;
    DELETE_COPY_CONSTRUCTOR(StaticBuffer)
    DELETE_COPY_OPERATOR(StaticBuffer)

private:
	StaticBuffer(D3dResource&& resource);
};

inline StaticBuffer::StaticBuffer(D3dResource&& resource) : D3dResource(std::move(resource)) { }

inline StaticBuffer::~StaticBuffer() = default;

inline uint64_t StaticBuffer::size() const
{
	return nativePtr()->GetDesc().Width;
}

inline StaticBuffer::StaticBuffer(StaticBuffer&& other) noexcept : D3dResource(std::move(other))
{
}

inline StaticBuffer& StaticBuffer::operator=(StaticBuffer&& other) noexcept
{
	D3dResource::operator=(std::move(other));
	return *this;
}
#endif