#pragma once
#ifdef WIN32
#include <Engine/pch.h>
#include <Engine/render/PC/RenderResource/FrameResource.h>
#include <Engine/render/PC/ScriptableRenderPath/RenderPass.h>

class RenderTexture;
class D3dGraphicContext;
struct RenderContext;
class Shader;
class D3dCommandList;
class DynamicBuffer;

class RenderObject
{
public:
    virtual void render(RenderContext& rc, D3dCommandList& d3dCmdList) = 0;
    virtual ~RenderObject() = 0;
    RenderObject() = delete;
    
    DELETE_MOVE_CONSTRUCTOR(RenderObject)
    DELETE_COPY_CONSTRUCTOR(RenderObject)
    DELETE_MOVE_OPERATOR(RenderObject)
    DELETE_COPY_OPERATOR(RenderObject)
};

class RenderPath
{
public:
    virtual void setUp(FrameResourceAllocator& allocator) = 0;
    virtual void render(D3dCommandList& cmdList) = 0;
    virtual void release() = 0;
    virtual ~RenderPath() = default;
    RenderPath() = default;
    
    DELETE_MOVE_CONSTRUCTOR(RenderPath)
    DELETE_COPY_CONSTRUCTOR(RenderPath)
    DELETE_MOVE_OPERATOR(RenderPath)
    DELETE_COPY_OPERATOR(RenderPath)
};

class SampleRenderPath : public RenderPath
{
public:
    void setUp(FrameResourceAllocator& allocator) override
    {
        mSamplePass.allocResources(allocator);
    }
    void render(D3dCommandList& cmdList) override;
    void release() override;
    ~SampleRenderPath() override;
    SampleRenderPath();

    DELETE_MOVE_CONSTRUCTOR(SampleRenderPath)
    DELETE_COPY_CONSTRUCTOR(SampleRenderPath)
    DELETE_MOVE_OPERATOR(SampleRenderPath)
    DELETE_COPY_OPERATOR(SampleRenderPath)
    
private:
    SamplePass mSamplePass;
};
#endif