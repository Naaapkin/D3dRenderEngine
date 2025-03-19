#ifdef WIN32
#include <Engine/common/Exception.h>
#include <Engine/render/PC/Shader.h>
#include <Engine/render/PC/Core/RenderContext.h>
#include <Engine/render/PC/RenderResource/RenderTexture.h>

void RenderContext::setRenderTargets(RenderTexture* renderTargets, uint8_t numRenderTargets)
{
    if (mRenderTargets == renderTargets || numRenderTargets <= 0 || !renderTargets) return;
    mRenderTargets = renderTargets;
    mPsoDescriptor.NumRenderTargets = numRenderTargets;
    uint8_t sampleCount = 0xff; uint8_t sampleQuality = 0xff;
    for (int i = 0; i < numRenderTargets; ++i)
    {
        mPsoDescriptor.RTVFormats[i] = static_cast<DXGI_FORMAT>(renderTargets[i].Format());
        DXGI_SAMPLE_DESC sampleDesc = renderTargets[i].NativePtr()->GetDesc().SampleDesc;
        sampleCount = std::min<uint8_t>(sampleCount, sampleDesc.Count);
        sampleQuality = std::min<uint8_t>(sampleQuality, sampleDesc.Quality);
    }
    mPsoDescriptor.SampleDesc = {sampleCount, sampleQuality};
    mIsPsoDirty = true;
}

void RenderContext::setDepthStencilFormat(TextureFormat format)
{
    if (mPsoDescriptor.DSVFormat == static_cast<DXGI_FORMAT>(format)) return;
    mPsoDescriptor.DSVFormat = static_cast<DXGI_FORMAT>(format);
    mIsPsoDirty = true;
}

TextureFormat RenderContext::getDepthStencilFormat() const
{
    return static_cast<TextureFormat>(mPsoDescriptor.DSVFormat);
}

RenderTexture* RenderContext::getRenderTargets() const
{
    return mRenderTargets;
}

void RenderContext::useResource(D3dResource& resource, uint64_t subResourceIndex)
{
#ifdef DEBUG || _DEBUG
    ASSERT(resource.NativePtr(), TEXT("resource is invalid"));    
#endif
    auto count = resource.SubResourceCount();
    if (subResourceIndex != D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        mSubResourceStates.emplace(SubResource{&resource, subResourceIndex}, new ResourceState[]{ResourceState::UNKNOWN, ResourceState::UNKNOWN});
        return;
    }
#ifdef DEBUG || _DEBUG
    ASSERT(count > subResourceIndex, TEXT("resource index is out of bound"));    
#endif
    for (int i = 0; i < count; ++i)
    {
        mSubResourceStates.emplace(SubResource{&resource, subResourceIndex}, new ResourceState[]{ResourceState::UNKNOWN, ResourceState::UNKNOWN});
    }
}

void RenderContext::useResource(D3dResource& resource)
{
#ifdef DEBUG || _DEBUG
    ASSERT(resource.NativePtr(), TEXT("resource is invalid"));
#endif
    mResourceStates.emplace(&resource, new ResourceState[]{ResourceState::UNKNOWN, ResourceState::UNKNOWN});
}

void RenderContext::setVertexShader(const Shader& shader)
{
#ifdef DEBUG || _DEBUG
    ASSERT(shader.Type() == ShaderType::VERTEX_SHADER, TEXT("shader type is not vertex shader"));
#endif
    // we just read from the shader object, so we dont need to worry about the constness
    const ID3DBlob* bin = shader.Binary();
    if (mPsoDescriptor.VS.pShaderBytecode == bin) return;
    mPsoDescriptor.VS = {
        const_cast<const LPVOID>(const_cast<ID3DBlob*>(bin)->GetBufferPointer()),
        const_cast<ID3DBlob*>(bin)->GetBufferSize() };
    // build input layout from vew vertex shader
    ShaderParameter const* parameters;
    uint8_t numParameters;
    shader.GetShaderParameters(&parameters, numParameters);
    delete[] mPsoDescriptor.InputLayout.pInputElementDescs;
    D3D12_INPUT_ELEMENT_DESC* inputDesc = new D3D12_INPUT_ELEMENT_DESC[numParameters];
    for (uint8_t i = 0; i < numParameters; ++i)
    {
        inputDesc[i].SemanticName = parameters[i].mSemantic;
        inputDesc[i].SemanticIndex = parameters[i].mSemanticIndex;
        inputDesc[i].Format = parameters[i].mFormat;
        inputDesc[i].InputSlot = parameters[i].mInputSlot;
        inputDesc[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
        inputDesc[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }
    mPsoDescriptor.InputLayout = { inputDesc, numParameters };
    mIsPsoDirty = true;
}

void RenderContext::setPixelShader(const Shader& shader)
{
#ifdef DEBUG || _DEBUG
    ASSERT(shader.Type() == ShaderType::PIXEL_SHADER, TEXT("shader type is not pixel shader"));
#endif
    // we just read from the shader object, so we dont need to worry about the constness
    const ID3DBlob* bin = shader.Binary();
    if (mPsoDescriptor.PS.pShaderBytecode == bin) return;
    mPsoDescriptor.PS = {
        const_cast<const LPVOID>(const_cast<ID3DBlob*>(bin)->GetBufferPointer()),
        const_cast<ID3DBlob*>(bin)->GetBufferSize() };
    mIsPsoDirty = true;
}

void RenderContext::foreachResource(const ForeachGeneralResource& foreachGeneral, const ForeachSubResource& foreachSubResource) const
{
    for (auto generalResource : mResourceStates)
    {
        foreachGeneral(*generalResource.first, generalResource.second);
    }

    for (auto subResource : mSubResourceStates)
    {
        foreachSubResource(*subResource.first.mResource, subResource.first.subResourceIndex, subResource.second);
    }
}

ID3D12CommandQueue* RenderContext::getCommandQueue() const
{
    return mCommandQueue;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC RenderContext::defaultPipelineStateDesc()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC1(D3D12_DEFAULT);

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RasterizerState.FrontCounterClockwise = true;	// counter-clock wise
    psoDesc.SampleMask = 0xffffffff;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    psoDesc.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };
    return psoDesc;
}

void RenderContext::TransitionPostExecution()
{
    bool isCopyQueue = mCommandQueue->GetDesc().Type == D3D12_COMMAND_LIST_TYPE_COPY;
    for (auto& states : mSubResourceStates)
    {
        auto& resource = states.first;
        auto desc = resource.mResource->NativePtr()->GetDesc();
        bool isBufferOrSimultaneous = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
        resource.mResource->ResourceStates()[resource.subResourceIndex] = isCopyQueue || isBufferOrSimultaneous ?
            ResourceState::COMMON : states.second[1];
    }

    for (auto& states : mResourceStates)
    {
        auto& resource = *(states.first);
        auto desc = resource.NativePtr()->GetDesc();
        bool isBufferOrSimultaneous = desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER || desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
        memset(resource.ResourceStates(),
            static_cast<int>(isBufferOrSimultaneous || isCopyQueue ?
                ResourceState::COMMON : states.second[1]),
            resource.SubResourceCount() * sizeof(ResourceState));
    }

    mResourceStates.clear();
    mSubResourceStates.clear();
}

RenderContext::RenderContext(ID3D12CommandQueue* commandQueue) :
    mCommandQueue(commandQueue),
    mPsoDescriptor(),
    mIsPsoDirty(true),
    mRenderTargets(nullptr)
{
}

ResourceState* RenderContext::getSubResourceTransition(D3dResource* pResource, uint64_t subResourceIndex)
{
#ifdef DEBUG || _DEBUG
    ASSERT(pResource->SubResourceCount() > subResourceIndex, TEXT("subResourceIndex out of range"));
#endif
    const auto it = mSubResourceStates.find({pResource, subResourceIndex});
    if (it == mSubResourceStates.end()) return nullptr;
	return it->second;
}

ResourceState* RenderContext::getResourceStates(D3dResource* pResource)
{
	const auto it = mResourceStates.find(pResource);
	if (it == mResourceStates.end()) return nullptr;
	return it->second;
}

RenderContext::~RenderContext() = default;
#endif