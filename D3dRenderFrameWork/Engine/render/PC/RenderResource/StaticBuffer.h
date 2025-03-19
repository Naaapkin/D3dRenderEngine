#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/RenderResource/D3dResource.h>

class StaticBuffer : public D3dResource
{
	friend StaticBuffer gCreateStaticBuffer(uint64_t size);
	
public:
	using D3dResource::release;

	~StaticBuffer() override;
    
    DEFAULT_MOVE_CONSTRUCTOR(StaticBuffer)
    DELETE_COPY_CONSTRUCTOR(StaticBuffer)
    DELETE_COPY_OPERATOR(StaticBuffer)

private:
	StaticBuffer(D3dResource&& resource);
	DEFAULT_MOVE_OPERATOR(StaticBuffer)
};

StaticBuffer gCreateStaticBuffer(uint64_t size);
#endif