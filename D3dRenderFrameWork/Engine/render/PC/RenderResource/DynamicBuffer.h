#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/RenderResource/D3dResource.h>

class DynamicBuffer : public D3dResource
{
	friend DynamicBuffer gCreateDynamicBuffer(uint64_t size);
	
public:
	void updateRange(const byte* data, uint64_t startPos, uint64_t width) const;
	byte* mappedPointer() const;
	void release() override;
	~DynamicBuffer() override;
	
	DEFAULT_MOVE_CONSTRUCTOR(DynamicBuffer)
	DELETE_COPY_CONSTRUCTOR(DynamicBuffer)
	DELETE_COPY_OPERATOR(DynamicBuffer)
	
private:
	DynamicBuffer(D3dResource&& resource);

	DEFAULT_MOVE_OPERATOR(DynamicBuffer)
	byte* mBufferMapper;
};

DynamicBuffer gCreateDynamicBuffer(uint64_t size);
#endif