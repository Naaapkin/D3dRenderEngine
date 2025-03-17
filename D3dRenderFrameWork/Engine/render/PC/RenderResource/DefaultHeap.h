#pragma once

#ifdef WIN32
#include "pch.h"
#include "D3dResource.h"

class DefaultHeap : private D3dResource
{
	friend DefaultHeap CreateDefaultBuffer(uint64_t size);
	
public:
	using D3dResource::Release;

	~DefaultHeap() override;

    D3dResource* operator->();
    
    DEFAULT_MOVE_CONSTRUCTOR(DefaultHeap)
    DELETE_COPY_CONSTRUCTOR(DefaultHeap)
    DEFAULT_MOVE_OPERATOR(DefaultHeap)
    DELETE_COPY_OPERATOR(DefaultHeap)

private:
	DefaultHeap(D3dResource&& resource);

};

DefaultHeap CreateDefaultBuffer(uint64_t size);
#endif