#pragma once
#include <Engine/pch.h>
#include <Engine/common/helper.h>
#include <Engine/render/PC/Data.h>
#include <Engine/render/PC/Core/RenderContext.h>
#include <Engine/render/PC/RenderResource/FrameResource.h>

class RenderPass
{
public:
    virtual void allocResources(FrameResourceAllocator& allocationRecorder) = 0;
    virtual void render(RenderContext& rc, D3dCommandList& cmdList) = 0;
    virtual void release() = 0;
    virtual ~RenderPass() = 0;
    RenderPass() = default;
    
    DELETE_MOVE_CONSTRUCTOR(RenderPass)
    DELETE_COPY_CONSTRUCTOR(RenderPass)
    DELETE_MOVE_OPERATOR(RenderPass)
    DELETE_COPY_OPERATOR(RenderPass)
};

class SamplePass : public RenderPass
{
public:
    void allocResources(FrameResourceAllocator& allocationRecorder) override
    {
        mLightConstantsHandle = allocationRecorder.allocDynamicBuffer(AllocationType::PER_FRAME, AlignUpToMul<uint64_t, 256>()(sizeof(LightConstants)));
        // mMaterialConstantsHandle = allocationRecorder.allocDynamicBuffer(AllocationType::PER_FRAME, AlignUpToMul<uint64_t, 256>()());    // TODO: calculate material constants size
        // mStaticMeshBuffer = allocationRecorder.allocStaticBuffer(AllocationType::PERSISTENT, );   // TODO: get mesh buffer size
    }
    void render(RenderContext& rc, D3dCommandList& cmdList) override;
    void release() override;
    ~SamplePass() override;
    SamplePass();

    DELETE_MOVE_CONSTRUCTOR(SamplePass)
    DELETE_COPY_CONSTRUCTOR(SamplePass)
    DELETE_MOVE_OPERATOR(SamplePass)
    DELETE_COPY_OPERATOR(SamplePass)

private:
    uint64_t mLightConstantsHandle;
    uint64_t mMaterialConstantsHandle;
    StaticBuffer* mStaticMeshBuffer;
};