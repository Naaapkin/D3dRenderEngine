#pragma once
#ifdef WIN32
#include <Engine/render/PC/RenderResource/D3dResource.h>

enum class TextureFormat : uint8_t;
class RenderTexture;
class Shader;
class D3dCommandQueue;

struct RenderContext 
{
    struct SubResource;
    struct Hash;

    using ForeachGeneralResource = std::function<void(D3dResource&, ResourceState*)>;
    using ForeachSubResource = std::function<void(D3dResource&, uint64_t subResourceIndex, ResourceState*)>;

public:
    void setRenderTargets(RenderTexture* renderTargets, uint8_t numRenderTargets);
    void setDepthStencilFormat(TextureFormat format);
    RenderTexture* getRenderTargets() const;
    TextureFormat getDepthStencilFormat() const;
    void useResource(D3dResource& resource, uint64_t subResourceIndex);
    void useResource(D3dResource& resource);
    void setVertexShader(const Shader& shader);
    void setPixelShader(const Shader& shader);
    void foreachResource(const ForeachGeneralResource& foreachGeneral, const ForeachSubResource& foreachSubResource) const;
    ID3D12CommandQueue* getCommandQueue() const;
    ResourceState* getSubResourceTransition(D3dResource* pResource, uint64_t subResourceIndex);
    ResourceState* getResourceStates(D3dResource* pResource);
    ~RenderContext();

    DELETE_COPY_CONSTRUCTOR(RenderContext)
    DELETE_COPY_OPERATOR(RenderContext)
    DEFAULT_MOVE_CONSTRUCTOR(RenderContext)
    DEFAULT_MOVE_OPERATOR(RenderContext)

private:
    struct SubResource
    {
        SubResource(D3dResource* resource, uint64_t subResourceIndex) : mResource(resource), subResourceIndex(subResourceIndex) { }
        D3dResource* mResource;
        uint64_t subResourceIndex;

        bool operator==(const SubResource& other) const
        {
            return mResource == other.mResource && subResourceIndex == other.subResourceIndex;
        }
    };
    
    struct HashSubResource
    {
        size_t operator()(const SubResource& subResource) const
        {
            return std::hash<ID3D12Resource*>{}(subResource.mResource->NativePtr())
                ^ std::hash<uint64_t>{}(subResource.subResourceIndex);
        }
    };
    static D3D12_GRAPHICS_PIPELINE_STATE_DESC defaultPipelineStateDesc();

    void TransitionPostExecution();
    RenderContext(ID3D12CommandQueue* commandQueue);
    // RenderContext(D3dCommandQueue& commandQueue);

    RenderTexture* mRenderTargets;
    ID3D12CommandQueue* mCommandQueue;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDescriptor;
    bool mIsPsoDirty;
    std::unordered_map<D3dResource*, ResourceState*> mResourceStates;
    std::unordered_map<SubResource, ResourceState*, HashSubResource> mSubResourceStates;
};
#endif