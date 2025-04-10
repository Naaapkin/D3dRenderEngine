#pragma once
#ifdef WIN32
#include "Engine/pch.h"
#include "Engine/render/PC/Resource/D3dResource.h"

class StaticHeap final : public D3dResource
{
	friend class D3D12RHIFactory;
public:
	uint64_t size() const override;
	StaticHeap();
    StaticHeap(StaticHeap&& other) noexcept;
	~StaticHeap() override;
	StaticHeap& operator=(StaticHeap&& other) noexcept;
    DELETE_COPY_CONSTRUCTOR(StaticHeap)
    DELETE_COPY_OPERATOR(StaticHeap)

private:
	StaticHeap(D3dResource&& resource);
};

inline StaticHeap::StaticHeap(D3dResource&& resource) : D3dResource(std::move(resource)) { }

inline StaticHeap::~StaticHeap() = default;

inline uint64_t StaticHeap::size() const
{
	return nativePtr()->GetDesc().Width;
}

inline StaticHeap::StaticHeap() = default;

inline StaticHeap::StaticHeap(StaticHeap&& other) noexcept : D3dResource(std::move(other))
{
}

inline StaticHeap& StaticHeap::operator=(StaticHeap&& other) noexcept
{
	D3dResource::operator=(std::move(other));
	return *this;
}
#endif