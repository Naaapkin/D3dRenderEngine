#pragma once
#ifdef WIN32
#include "Engine/render/RHIDefination.h"
#include "Engine/render/PC/Private/D3D12RenderTarget.h"
#include "Engine/render/PC/Private/D3D12GraphicsContext.h"

class D3D12SwapChain : public RHISwapChain
{
public:
    RHIRenderTarget* GetCurrentColorTexture() override
    {
        return mColorBuffers[mBackBufferIndex];
    }
    RHIRenderTarget* GetColorTexture(uint8_t backBufferIndex) override
    {
        return mColorBuffers[backBufferIndex];
    }
    RHITextureDesc GetBackBufferDesc() const override
    {
        return mColorBuffers[0]->GetTextureDesc();
    }
    void BeginFrame(RHIGraphicsContext* pContext) override
    {
        D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
        pNativeContext->TransitionResource(static_cast<D3D12RenderTarget*>(mColorBuffers[mBackBufferIndex]), ResourceState::RENDER_TARGET);
    }
    void EndFrame(RHIGraphicsContext* pContext) override
    {
        D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
        pNativeContext->TransitionResource(static_cast<D3D12RenderTarget*>(mColorBuffers[mBackBufferIndex]), ResourceState::PRESENT);
    }
    void Present() override
    {
        mSwapChain->Present(1, 0);
        RHISwapChain::Present();
    }

    D3D12SwapChain() = default;

    D3D12SwapChain(UComPtr<IDXGISwapChain1> pSwapChain, std::unique_ptr<RHIRenderTarget*[]> colorBuffers,
                   uint8_t numBackBuffers) : RHISwapChain(numBackBuffers), mSwapChain(std::move(pSwapChain)),
                                             mColorBuffers(std::move(colorBuffers))
    { }

    ~D3D12SwapChain() override
    {
	    for (int i = 0; i < mNumBackBuffers; ++i)
	    {
            static_cast<D3D12RenderTarget*>(mColorBuffers[i])->mResource.Detach();
	    }
    }

private:
    UComPtr<IDXGISwapChain1> mSwapChain;
    std::unique_ptr<RHIRenderTarget*[]> mColorBuffers;
};
#endif