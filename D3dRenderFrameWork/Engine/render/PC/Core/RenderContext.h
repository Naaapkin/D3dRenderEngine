#pragma once
#ifdef WIN32

class StaticHeap;
class D3D12CommandList;
enum class Format : uint8_t;
class RenderTexture2D;
class HlslShader;
class D3dCommandQueue;

struct RenderContext 
{
public:
    void setPassCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex);
    void setObjectCbvStartIdx(D3D12_CPU_DESCRIPTOR_HANDLE cbvStartIndex);
    void setRenderTargets(const StaticHeap* renderTargets, uint8_t numRenderTargets);
    void setDepthStencilBuffer(const StaticHeap* depthStencilBuffer);
    const StaticHeap* backBuffer() const;
    const StaticHeap* depthStencilBuffer() const;
    void setCommandQueue(ID3D12CommandQueue* pCommandQueue);
    void executeCommandLists(const std::vector<D3D12CommandList*>& commandLists) const;
    void executeCommandList(D3D12CommandList* commandList) const;
    void reset(ID3D12CommandQueue* pCommandQueue);

    RenderContext();
    ~RenderContext();

    DELETE_COPY_CONSTRUCTOR(RenderContext)
    DELETE_COPY_OPERATOR(RenderContext)
    DEFAULT_MOVE_CONSTRUCTOR(RenderContext)
    DEFAULT_MOVE_OPERATOR(RenderContext)

private:    
    ID3D12CommandQueue* mCommandQueue;
    const StaticHeap* mRenderTargets;
    const StaticHeap* mDepthStencilBuffer;
    uint8_t mNumTargetRenderTarget;
    D3D12_CPU_DESCRIPTOR_HANDLE mPassCbvStartIdx;
    D3D12_CPU_DESCRIPTOR_HANDLE mObjectCbvStartIdx;
};
#endif