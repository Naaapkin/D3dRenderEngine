#pragma once
#ifdef WIN32
#include "pch.h"
#include "D3dResource.h"

class UploadHeap : private D3dResource
{
	friend UploadHeap CreateUploadBuffer(uint64_t size);
	
public:
	void UpdateRange(const byte* data, uint64_t startPos, uint64_t width);
	byte* MappedPointer() const;
	void Release() override;
	~UploadHeap() override;

	DEFAULT_MOVE_CONSTRUCTOR(UploadHeap)
	DEFAULT_MOVE_OPERATOR(UploadHeap)
	DELETE_COPY_CONSTRUCTOR(UploadHeap)
	DELETE_COPY_OPERATOR(UploadHeap)

	D3dResource* operator->();

private:
	UploadHeap(D3dResource&& resource);
	byte* mBufferMapper;
};

UploadHeap CreateUploadBuffer(uint64_t size);
#endif