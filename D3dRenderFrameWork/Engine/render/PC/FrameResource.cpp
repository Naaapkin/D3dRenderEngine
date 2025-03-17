#ifdef WIN32
#include <Engine/common/PC/WException.h>
#include <Engine/render/PC/D3dGraphicContext.h>
#include <Engine/render/PC/Data.h>
#include <Engine/render/PC/FrameResource.h>
#include <Engine/render/PC/RenderResource/UploadHeap.h>

UploadHeap* FrameResource::PassConstantsBuffer() const
{
    return mPassCB.get();
}

UploadHeap* FrameResource::ObjectConstantsBuffer() const
{
    return mObjectCB.get();
}

FrameResource::FrameResource(
    ID3D12CommandAllocator* pCommandAllocator,
    uint16_t passCount,
    uint16_t objectCount) :
    mCommandAllocator(pCommandAllocator),
    mPassCB(new UploadHeap(::CreateUploadBuffer(passCount * sizeof(Constant<PassConstants>)))),
    mObjectCB(new UploadHeap(::CreateUploadBuffer(objectCount * sizeof(Constant<ObjectConstants>)))) { }

FrameResource CreateFrameResource(uint16_t passCount, uint16_t objectCount)
{
    ID3D12Device* pDevice = gGraphicContext()->DeviceHandle();
    ID3D12CommandAllocator* pCommandAllocator;
    ThrowIfFailed(
        pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&pCommandAllocator))
    );
    return { pCommandAllocator, passCount, objectCount };
}
#endif