#pragma once
#ifdef WIN32
#include <Engine/render/PC/RenderResource/D3dResource.h>

class Shader;
class D3dCommandQueue;

struct RenderContext 
{
    struct SubResource;
    struct Hash;

    using ForeachGeneralResource = std::function<void(D3dResource&, ResourceState*)>;
    using ForeachSubResource = std::function<void(D3dResource&, uint64_t subResourceIndex, ResourceState*)>;

public:
    void UseResource(D3dResource& resource, uint64_t subResourceIndex);
    void UseResource(D3dResource& resource);
    void SetVertexShader(const Shader& shader);
    void SetPixelShader(const Shader& shader);
    void ForeachResource(const ForeachGeneralResource& foreachGeneral, const ForeachSubResource& foreachSubResource) const;
    D3dCommandQueue* GetCommandQueue() const;
    ResourceState* GetSubResourceTransition(D3dResource* pResource, uint64_t subResourceIndex);
    ResourceState* GetResourceStates(D3dResource* pResource);
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

    void TransitionPostExecution();
    RenderContext(D3dCommandQueue& commandQueue);

    D3dCommandQueue* mCommandQueue;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC mPsoDescriptor;
    bool mIsPsoDirty;
    std::unordered_map<D3dResource*, ResourceState*> mResourceStates;
    std::unordered_map<SubResource, ResourceState*, HashSubResource> mSubResourceStates;
};
#endif