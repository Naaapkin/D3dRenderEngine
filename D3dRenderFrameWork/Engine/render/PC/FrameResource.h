#pragma once
#ifdef WIN32
#include <pch.h>

struct ObjectConstants;
struct PassConstants;
struct Fence;
class UploadHeap;
class FrameResource;

FrameResource CreateFrameResource(uint16_t passCount, uint16_t objectCount);

class FrameResource
{
    friend FrameResource CreateFrameResource(uint16_t passCount, uint16_t objectCount);
    
public:
    UploadHeap* PassConstantsBuffer() const;
    UploadHeap* ObjectConstantsBuffer() const;
    
private:
    FrameResource(ID3D12CommandAllocator* pCommandAllocator, uint16_t passCount, uint16_t objectCount);

    ComPtr<ID3D12CommandAllocator> mCommandAllocator;
    std::unique_ptr<UploadHeap> mPassCB;
    std::unique_ptr<UploadHeap> mObjectCB;
    uint64_t mFenceValue = 0;
};
#endif