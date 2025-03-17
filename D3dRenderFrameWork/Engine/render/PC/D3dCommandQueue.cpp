#ifdef WIN32
#include <Engine/common/PC/WException.h>
#include <Engine/render/PC/D3dCommandList.h>
#include <Engine/render/PC/D3dCommandQueue.h>
#include <Engine/render/PC/D3dGraphicContext.h>


D3dCommandList D3dCommandQueue::CreateCommandList(ID3D12CommandAllocator* pCommandAllocator) const
{
    return ::CreateD3dCommandList(::gGraphicContext()->DeviceHandle(), pCommandAllocator, GetQueueType());
}

D3D12_COMMAND_LIST_TYPE D3dCommandQueue::GetQueueType() const
{
#ifdef DEBUG || _DEBUG
    ASSERT(mCommandQueue, TEXT("command queue is null\n"))
#endif
    return mCommandQueue->GetDesc().Type;
}

D3dCommandQueue::~D3dCommandQueue() = default;

D3dCommandQueue::D3dCommandQueue(ID3D12CommandQueue* commandQueue) : mCommandQueue(commandQueue) { }
#endif
