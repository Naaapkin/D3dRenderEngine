#include "RenderPass.h"

RenderPass::~RenderPass() { }

SamplePass::SamplePass() : mLightConstantsHandle(NULL), mMaterialConstantsHandle(NULL), mStaticMeshBuffer(nullptr) { }

void SamplePass::render(RenderContext& rc, D3dCommandList& cmdList)
{
}

void SamplePass::release()
{
    delete mStaticMeshBuffer;
    mStaticMeshBuffer = nullptr;
}

SamplePass::~SamplePass()
{
    SamplePass::release();
}
