#pragma once

#ifdef WIN32
#include <pch.h>

class D3dCommandList;

class D3dCommandQueue
{
	friend D3dCommandQueue CreateCommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);

public:
	D3dCommandList CreateCommandList(ID3D12CommandAllocator* pCommandAllocator) const;
	D3D12_COMMAND_LIST_TYPE GetQueueType() const;
	~D3dCommandQueue();

	DELETE_COPY_CONSTRUCTOR(D3dCommandQueue)
	DELETE_COPY_OPERATOR(D3dCommandQueue)
	DEFAULT_MOVE_CONSTRUCTOR(D3dCommandQueue)
	DEFAULT_MOVE_OPERATOR(D3dCommandQueue)

private:
	D3dCommandQueue(ID3D12CommandQueue* commandQueue);

	ComPtr<ID3D12CommandQueue> mCommandQueue;
};

D3dCommandQueue CreateCommandQueue(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
#endif