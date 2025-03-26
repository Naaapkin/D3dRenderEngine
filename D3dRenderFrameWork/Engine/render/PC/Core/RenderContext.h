#pragma once
#ifdef WIN32

class D3dCommandList;
enum class TextureFormat : uint8_t;
class RenderTexture2D;
class Shader;
class D3dCommandQueue;

struct RenderContext 
{
public:
    void setPassCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex);
    void setObjectCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex);
    void setRenderTargets(const RenderTexture2D* renderTargets, uint8_t numRenderTargets);
    void setDepthStencilBuffer(const RenderTexture2D* depthStencilBuffer);
    const RenderTexture2D* backBuffer() const;
    const RenderTexture2D* depthStencilBuffer() const;
    void setCommandQueue(ID3D12CommandQueue* pCommandQueue);
    void executeCommandLists(const std::vector<D3dCommandList*>& commandLists) const;
    void executeCommandList(D3dCommandList*& commandList) const;
    void reset(ID3D12CommandQueue* pCommandQueue);

    RenderContext();
    ~RenderContext();

    DELETE_COPY_CONSTRUCTOR(RenderContext)
    DELETE_COPY_OPERATOR(RenderContext)
    DEFAULT_MOVE_CONSTRUCTOR(RenderContext)
    DEFAULT_MOVE_OPERATOR(RenderContext)

private:    
    ID3D12CommandQueue* mCommandQueue;
    std::vector<D3dCommandList*> mPendingCommandLists;
    const RenderTexture2D* mRenderTargets;
    const RenderTexture2D* mDepthStencilBuffer;
    uint8_t mNumTargetRenderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE mPassCbvStartIdx;
    D3D12_CPU_DESCRIPTOR_HANDLE mObjectCbvStartIdx;
};
#endif