#pragma once
#ifdef WIN32
#include "Engine/Render/RHIDefination.h"
#include "Engine/Render/PC/Private/D3D12GraphicsContext.h"

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
        return mColorBuffers[0]->GetBuffer()->GetDesc();
    }
    void BeginFrame(RHIGraphicsContext* pContext) override
    {
        D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
        D3D12Texture* pNativeTexture = static_cast<D3D12Texture*>(mColorBuffers[mBackBufferIndex]->GetBuffer());
        pNativeContext->TransitionResource(pNativeTexture->GetD3D12Resource(), ResourceState::RENDER_TARGET);
    }
    void EndFrame(RHIGraphicsContext* pContext) override
    {
        D3D12GraphicsContext* pNativeContext = static_cast<D3D12GraphicsContext*>(pContext);
        D3D12Texture* pNativeTexture = static_cast<D3D12Texture*>(mColorBuffers[mBackBufferIndex]->GetBuffer());
        pNativeContext->TransitionResource(pNativeTexture->GetD3D12Resource(), ResourceState::PRESENT);
    }
    void Present() override
    {
        mSwapChain->Present(1, 0);
        RHISwapChain::Present();
    }

    D3D12SwapChain() = default;

    D3D12SwapChain(UComPtr<IDXGISwapChain1> pSwapChain, D3D12FrameBuffer** colorBuffers,
                   uint8_t numBackBuffers) : RHISwapChain(numBackBuffers), mSwapChain(std::move(pSwapChain))
    {
        mColorBuffers.reset(new RHIRenderTarget * [numBackBuffers]);
	    for (uint8_t i = 0; i < numBackBuffers; ++i)
	    {
            mColorBuffers[i] = new RHIRenderTarget(std::unique_ptr<D3D12FrameBuffer>(colorBuffers[i]));
	    }
    }

    ~D3D12SwapChain() override
    {
	    for (int i = 0; i < mNumBackBuffers; ++i)
	    {
            D3D12Texture* pNativeTexture = static_cast<D3D12Texture*>(mColorBuffers[i]->GetBuffer());
            pNativeTexture->mResource.Detach();
            delete mColorBuffers[i];
	    }
        mColorBuffers.reset();
    }

private:
    UComPtr<IDXGISwapChain1> mSwapChain;
    std::unique_ptr<RHIRenderTarget*[]> mColorBuffers;
};
#endif