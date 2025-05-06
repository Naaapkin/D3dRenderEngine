#include "Renderer.h"

RenderTargetRef RenderTargetHandle::GetRenderTargetRef() const { return mRTRef; }

uint32_t RenderTargetHandle::GetWidth() const { return mDesc.mWidth; }

uint32_t RenderTargetHandle::GetHeight() const { return mDesc.mHeight; }

Format RenderTargetHandle::GetFormat() const { return mDesc.mFormat; }

bool RenderTargetHandle::MSAAEnabled() const { return mDesc.mSampleCount > 1; }

RenderTargetHandle::RenderTargetHandle() = default;

RenderTargetHandle::RenderTargetHandle(RenderTargetRef renderTargetRef, const RHITextureDesc& desc): mRTRef(renderTargetRef), mDesc(desc)
{ }

DepthStencilRef DepthStencilHandle::GetRenderTargetRef() const { return mDSRef; }

uint32_t DepthStencilHandle::GetWidth() const { return mDesc.mWidth; }

uint32_t DepthStencilHandle::GetHeight() const { return mDesc.mHeight; }

Format DepthStencilHandle::GetFormat() const { return mDesc.mFormat; }

bool DepthStencilHandle::MSAAEnabled() const { return mDesc.mSampleCount > 1; }

bool DepthStencilHandle::MipmapEnabled() const { return mDesc.mMipLevels > 1; }

DepthStencilHandle::DepthStencilHandle() = default;

DepthStencilHandle::DepthStencilHandle(RenderTargetRef depthStencilRef, const RHITextureDesc& desc): mDSRef(depthStencilRef), mDesc(desc)
{ }

std::unique_ptr<Material> Renderer::createMaterial(RHIShader* pShader)
{
	ASSERT(pShader, TEXT("mShader must not be null."));
	uint32_t numCBuffers = 0;
	uint32_t numTextures = 0;
	uint32_t numSamplers = 0;
	const std::vector<ShaderProp>& properties = pShader->GetShaderProperties();
	for (auto& shaderProp : properties)
	{
		switch (shaderProp.mType)
		{
		case ShaderPropType::CBUFFER:
			numCBuffers += shaderProp.mRegister >= 2;	// TODO: magic number.
			break;
		case ShaderPropType::TEXTURE:
			numTextures++;
			break;
		case ShaderPropType::SAMPLER:
			numSamplers++;
			break;
		}
	}
	Material* material = new Material();
	material->mNumConstants = numCBuffers;
	material->mNumTextures = numTextures;
	material->mNumSamplers = numSamplers;
	material->mConstants.reset(new ConstantProperty[numCBuffers]);
	material->mTextures.reset(new TextureProperty[numTextures]);
	material->mSamplers.reset(new SamplerProperty[numSamplers]);
	material->mShader = pShader;
	numCBuffers = 0;
	numTextures = 0;
	numSamplers = 0;
	for (uint32_t i = 0; i < properties.size(); ++i)
	{
		const ShaderProp& prop = properties[i];
#ifdef UNICODE
		std::wstring&& propName = ::AsciiToUtf8(prop.mName);
#else
		std::string& propName = prop.mName;
#endif
		if (prop.mType == ShaderPropType::CBUFFER && prop.mRegister >= 2)
		{
			material->mConstants[numCBuffers] = ConstantProperty{ propName, prop.mRegister, prop.mInfo.mCBufferSize };
			numCBuffers++;
		}
		else if (prop.mType == ShaderPropType::TEXTURE)
		{
			material->mTextures[numTextures] = TextureProperty{ propName, prop.mRegister, prop.mInfo.mTextureDimension };
			numTextures++;
		}
		else if (prop.mType == ShaderPropType::SAMPLER)
		{
			//material->mSamplers[numSamplers] = { propName };
			numSamplers++;
		}
	}
	return std::unique_ptr<Material>(material);

}

std::unique_ptr<MaterialInstance> Renderer::createMaterialInstance(const Material& material)
{
	static uint64_t id = 0;
	MaterialInstance* materialInstance = new MaterialInstance{};
	materialInstance->SetMaterialInstanceId(id++);
	materialInstance->InstantiateFrom(&material);
	return std::unique_ptr<MaterialInstance>(materialInstance);
}

void Renderer::initialize()
{
	uint8_t numBackBuffer = 3;

	// initialize render hardware interface(rhi).
#ifdef WIN32
	RHI* rhi = new D3D12RHI();
#else
	RHI& rhi = new PlayStationRHI();
#endif
	RHIConfiguration rhiConfiguration = RHIConfiguration::Default();
	rhi->Initialize(rhiConfiguration);
        
	RHISwapChainDesc swapChainDesc{};
	swapChainDesc.mFormat = Format::R8G8B8A8_UNORM;
	swapChainDesc.mWidth = 0;
	swapChainDesc.mHeight = 0;
	swapChainDesc.mNumBackBuffers = numBackBuffer;
	swapChainDesc.mIsFullScreen = false;
	swapChainDesc.mMSAA = 1;
	swapChainDesc.mTargetFramesPerSec = 0;   // use zero to force the native display's refresh rate. 
	mSwapChain = rhi->RHICreateSwapChain(swapChainDesc);

	// Create depth-stencil buffer
	RHITextureDesc depthTextureDesc = mSwapChain->GetBackBufferDesc(); 
	depthTextureDesc.mFormat = Format::D24_UNORM_S8_UINT;
	mDepthStencilBuffer = rhi->RHIAllocDepthStencil(depthTextureDesc).release();
	mGPUResources.emplace_back(mDepthStencilBuffer);

	// Create render contexts, number of render contexts must not be larger than back buffer count.
	mNumRenderContexts = numBackBuffer;
	mCurrentRenderContextIndex = 0;
	RHIGraphicsContext* pRenderContext;
	mRenderContexts.reset(new RenderContext[numBackBuffer]);
	for (uint8_t i = 0; i < numBackBuffer; ++i)
	{
		rhi->CreateGraphicsContext(&pRenderContext);
		mRenderContexts[i].mGraphicContext = std::unique_ptr<RHIGraphicsContext>(pRenderContext);
		mRenderContexts[i].mFenceGPU = rhi->RHICreateFence();
		mRenderContexts[i].mFenceCPU = 0;
	}

	RHICopyContext* pCopyContext;
	rhi->CreateCopyContext(&pCopyContext);
	mCopyContext.reset(pCopyContext);
	mCopyFenceGPU = rhi->RHICreateFence();
	mCopyFenceCPU = 0;

	mRenderHardwareInterface = rhi;

	createBuiltinResources();
        
	// prepare pipeline states
	mPipeStateInitializers.resize(NUM_PRESETS);
	PipelineInitializer& preDepth = mPipeStateInitializers[PSO_PRE_DEPTH];
	preDepth = PipelineInitializer::Default();
	static const auto it = mShaderMap.find(TEXT("PreDepth"));
	ASSERT(it != mShaderMap.end(), TEXT("missing mShader : PreDepth"));
	preDepth.SetShader(it->second);
	preDepth.SetBlend(false, false, BlendDesc::Disabled());


	PipelineInitializer& shadow = mPipeStateInitializers[PSO_SHADOW];
	shadow = PipelineInitializer::Default();

	PipelineInitializer& opaque = mPipeStateInitializers[PSO_OPAQUE];
	opaque = PipelineInitializer::Default();
	opaque.SetBlend(false, false, BlendDesc::Disabled());

	PipelineInitializer& transparent = mPipeStateInitializers[PSO_TRANSPARENT];
	transparent = PipelineInitializer::Default();
}

VertexBufferRef Renderer::allocVertexBuffer(uint32_t numVertices, uint32_t vertexSize)
{
	auto&& pVertexBuffer = mRenderHardwareInterface->RHIAllocVertexBuffer(vertexSize, numVertices);
	if (mAvailableGPUResourceIds.empty())
	{
		mGPUResources.emplace_back(std::move(pVertexBuffer));
		return mGPUResources.size() - 1;
	}
	uint64_t index = mAvailableGPUResourceIds.top();
	mAvailableGPUResourceIds.pop();
	mGPUResources[index] = std::move(pVertexBuffer);
	return index;
}

IndexBufferRef Renderer::allocIndexBuffer(uint32_t numIndices, Format indexFormat)
{
	auto&& pIndexBuffer = mRenderHardwareInterface->RHIAllocIndexBuffer(numIndices, indexFormat);
	if (mAvailableGPUResourceIds.empty())
	{
		mGPUResources.emplace_back(std::move(pIndexBuffer));
		return mGPUResources.size() - 1;
	}
	uint64_t index = mAvailableGPUResourceIds.top();
	mAvailableGPUResourceIds.pop();
	mGPUResources[index] = std::move(pIndexBuffer);
	return index;
}

TextureRef Renderer::allocTexture2D(Format format, uint32_t width, uint32_t height, uint8_t mipLevels)
{
	auto&& pTexture = mRenderHardwareInterface->RHIAllocTexture({ format, TextureDimension::TEXTURE2D, width, height, 1, mipLevels, 1, 0 });
	if (mAvailableGPUResourceIds.empty())
	{
		mGPUResources.emplace_back(std::move(pTexture));
		return mGPUResources.size() - 1;
	}
	uint64_t index = mAvailableGPUResourceIds.top();
	mAvailableGPUResourceIds.pop();
	mGPUResources[index] = std::move(pTexture);
	return index;
}

void Renderer::updateVertexBuffer(const void* pData, uint64_t bufferSize, VertexBufferRef vertexBufferGPU, bool blockRendering)
{
	RHIVertexBuffer* pVertexBuffer = static_cast<RHIVertexBuffer*>(mGPUResources[vertexBufferGPU].get());
	uint64_t size = std::min<uint64_t>(pVertexBuffer->GetBuffer()->BufferSize(), bufferSize);
	auto&& stagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(size);
	mRenderHardwareInterface->UpdateStagingBuffer(stagingBuffer.get(), pData, 0, size);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->ResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateBuffer(pVertexBuffer, stagingBuffer.get(), size, 0, 0);
	mRenderHardwareInterface->SubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->SyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

void Renderer::updateIndexBuffer(const void* pData, uint64_t bufferSize, IndexBufferRef indexBufferGPU, bool blockRendering)
{
	RHIIndexBuffer* pIndexBuffer = static_cast<RHIIndexBuffer*>(mGPUResources[indexBufferGPU].get());
	uint64_t size = std::min<uint64_t>(pIndexBuffer->GetBuffer()->BufferSize(), bufferSize);
	auto&& stagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(size);
	mRenderHardwareInterface->UpdateStagingBuffer(stagingBuffer.get(), pData, 0, size);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->ResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateBuffer(pIndexBuffer, stagingBuffer.get(), size, 0, 0);
	mRenderHardwareInterface->SubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->SyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

void Renderer::updateTexture(const void* pData, TextureRef textureGPU, bool blockRendering)
{
	RHINativeTexture* pTexture = static_cast<RHINativeTexture*>(mGPUResources[textureGPU].get());
	const RHITextureDesc& desc = pTexture->GetDesc();
	uint64_t size = desc.mWidth * desc.mHeight * desc.mDepth;
	// TODO: support update multi-mips
	std::unique_ptr<RHIStagingBuffer> pStagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(size);
	mRenderHardwareInterface->UpdateStagingBuffer(pStagingBuffer.get(), pData, 0, size);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->ResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateTexture(pTexture, pStagingBuffer.get(), 0);
	mRenderHardwareInterface->SubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->SyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

std::unique_ptr<RHIShader> Renderer::compileShader(const Blob& blob, ShaderType shaderTypes, const String* path) const
{
#ifdef UNICODE
	std::unique_ptr<RHIShader> shader;
	if (path)
	{
		const std::string pathStr = ::Utf8ToAscii(*path);
		shader = mRenderHardwareInterface->RHICompileShader(blob, shaderTypes, &pathStr);
	}
	else
	{
		shader = mRenderHardwareInterface->RHICompileShader(blob, shaderTypes);
	}
#else
    std::unique_ptr<RHIShader> mShader = mRenderHardwareInterface->RHICompileShader(blob, shaderTypes, path);
#endif
	return shader;
}

RHIShader* Renderer::registerShader(const String& name, std::unique_ptr<RHIShader>&& shader)
{

	mShaderRegisterMutex.lock_shared();
#ifdef UNICODE
	shader->SetName(::Utf8ToAscii(name));
#else
        mShader->SetName(name);
#endif
	const auto it = mShaderMap.find(name);
	if (it != mShaderMap.end()) return it->second;
	mShaderRegisterMutex.unlock_shared();

	RHIShader* pShader = shader.release();
	mShaderRegisterMutex.lock();
	if (!mAvailableShaderId.empty())
	{
		mShaderMap.emplace(name, pShader);
		mShaders[mAvailableShaderId.top()].reset(pShader);
		mAvailableShaderId.pop();
	}
	else
	{
		mShaderMap.emplace(name, pShader);
		mShaders.emplace_back(pShader);
	}
	mShaderRegisterMutex.unlock();
	return pShader;
}

void Renderer::render()
{
	RenderContext& renderContext = mRenderContexts[mCurrentRenderContextIndex];
	RHIGraphicsContext* graphicsContext = renderContext.mGraphicContext.get();
	renderContext.mFenceGPU->Wait(renderContext.mFenceCPU);
	mRenderHardwareInterface->ResetGraphicsContext(graphicsContext);
	mSwapChain->BeginFrame(graphicsContext);

	for (auto& renderList : mRenderLists)
	{
		// TODO: support specify the render target and depth stencil.
		RHIRenderTarget* pRenderTarget = mSwapChain->GetCurrentColorTexture();
		RHIDepthStencil* pDepthStencil = mDepthStencilBuffer;
		RHITextureDesc rtDesc = pRenderTarget->GetTextureDesc();
		Viewport viewports[] = { Viewport{static_cast<float>(rtDesc.mWidth), static_cast<float>(rtDesc.mHeight), 0, 1} };
		Rect scissorRects[] = { Rect{0, 0, static_cast<int32_t>(rtDesc.mWidth), static_cast<int32_t>(rtDesc.mHeight)} };
		// TODO: Calculations like this can move to pre-render phase.

		graphicsContext->SetViewPorts(viewports, 1);
		graphicsContext->SetScissorRect(scissorRects, 1);

		graphicsContext->SetRenderTargetsAndDepthStencil(nullptr, 0, pDepthStencil);
		graphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, 0, nullptr, 0);

		mPipeStateInitializers[PSO_PRE_DEPTH].SetFrameBuffers(Format::UNKNOWN, mDepthStencilBuffer->GetFormat());
		mPipeStateInitializers[PSO_PRE_DEPTH].SetDepthStencil(mDepthStencilBuffer->GetFormat());

		depthPrePass(graphicsContext, renderList.mOpaqueList, renderList.mCameraConstants);
		//mRenderHardwareInterface->SubmitRenderCommands(graphicsContext);

		//mRenderHardwareInterface->ResetGraphicsContext(graphicsContext);
		graphicsContext->SetViewPorts(viewports, 1);
		graphicsContext->SetScissorRect(scissorRects, 1);

		graphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
		graphicsContext->ClearRenderTarget(pRenderTarget, renderList.mBackGroundColor, scissorRects, 1);
		graphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, 0, scissorRects, 1);

		opaquePass(graphicsContext, renderList.mOpaqueList, renderList.mCameraConstants);
	}
	mSwapChain->EndFrame(graphicsContext);
	mRenderHardwareInterface->SubmitRenderCommands(graphicsContext);
	mRenderHardwareInterface->SyncGraphicContext(renderContext.mFenceGPU.get(), ++renderContext.mFenceCPU);
	mSwapChain->Present();
	mCurrentRenderContextIndex = (mCurrentRenderContextIndex + 1) % mNumRenderContexts;

	mRenderLists.clear();
}

Renderer::Renderer() = default;

void Renderer::createBuiltinResources()
{
	const char* builtinShaderSources[] = {
		"cbuffer ObjectConstants : register(b1) { float4x4 m_model; float4x4 m_view; float4x4 m_projection; float4x4 m_model_i; float4x4 m_view_i; float4x4 m_projection_i; }; struct SimpleVertexInput{float3 position : POSITION;float3 normal : NORMAL;float2 uv : TEXCOORD;};struct FragInput{float4 position : SV_POSITION;};FragInput VsMain(SimpleVertexInput input){FragInput o;float4 worldPosition = mul(m_model, float4(input.position, 1));o.position = mul(m_projection, mul(m_view, worldPosition));return o;}",
	};

	Blob shaderSource{builtinShaderSources[0], strlen(builtinShaderSources[0])};
	std::unique_ptr<RHIShader> pShader = compileShader(shaderSource, ShaderType::VERTEX);
	registerShader(TEXT("PreDepth"), std::move(pShader));
}

void Renderer::beginFrame(RHIGraphicsContext* renderContext)
{
	mRenderHardwareInterface->ResetGraphicsContext(renderContext);
}

void Renderer::depthPrePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants)
{
	PipelineInitializer& preDepthPSO = mPipeStateInitializers[PSO_PRE_DEPTH];
	TransformConstants transform{DirectX::XMMatrixIdentity(), cameraConstants.mView, cameraConstants.mProjection,
	DirectX::XMMatrixIdentity(),cameraConstants.mViewInverse,  cameraConstants.mProjectionInverse };
	for (const auto& renderItem : renderItems)
	{
		pRenderContext->BeginDrawCall();
		const MaterialInstance& material = *renderItem.mMaterial;
		const DepthTestDesc& depthTest = renderItem.mMaterial->DepthTest();
		if (!depthTest.mEnableDepthTest) continue;

		preDepthPSO.SetCullMode(material.GetCullMode());
		preDepthPSO.SetDrawMode(renderItem.mMaterial->GetDrawMode());
		preDepthPSO.SetDepthTest(depthTest);
		preDepthPSO.SetStencilTest(material.StencilTest());
		pRenderContext->SetPipelineState(preDepthPSO);

		transform.mModel = renderItem.mModel;
		transform.mModelInverse = renderItem.mModelInverse;
		std::unique_ptr<RHIConstantBuffer> cbuffer = mRenderHardwareInterface->RHIAllocConstantBuffer(sizeof(TransformConstants));
		mRenderHardwareInterface->UpdateConstantBuffer(cbuffer.get(), &transform, 0, sizeof(TransformConstants));
		pRenderContext->SetConstantBuffer(1, cbuffer.get());

		RHIVertexBuffer* vertexBuffers[] = { static_cast<RHIVertexBuffer*>(mGPUResources[renderItem.mMeshData.mVertexBuffer].get()) };
		pRenderContext->SetVertexBuffers(vertexBuffers, 1);
		pRenderContext->SetIndexBuffer(static_cast<RHIIndexBuffer*>(mGPUResources[renderItem.mMeshData.mIndexBuffer].get()));
		const SubMesh* subMeshes = renderItem.mMeshData.mSubMeshes.get();
		uint32_t numSubMeshes = renderItem.mMeshData.mSubMeshCount;
		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			// TODO: support gpu instancing.
			pRenderContext->DrawIndexedInstanced(subMeshes[i].mIndexNum, subMeshes[i].mStartIndex, 1);
		}
		pRenderContext->EndDrawCalls();
	}
}

void Renderer::opaquePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants)
{
	PipelineInitializer& opaquePSO = mPipeStateInitializers[PSO_OPAQUE];
	TransformConstants transform{ DirectX::XMMatrixIdentity(), cameraConstants.mView, cameraConstants.mProjection,
		DirectX::XMMatrixIdentity(),cameraConstants.mViewInverse,  cameraConstants.mProjectionInverse };
	for (const auto& renderItem : renderItems)
	{
		pRenderContext->BeginDrawCall();

		// set pipeline states
		const MaterialInstance& material = *renderItem.mMaterial;
		opaquePSO.SetCullMode(material.GetCullMode());
		opaquePSO.SetDrawMode(material.GetDrawMode());
		opaquePSO.SetDepthTest(material.DepthTest());
		opaquePSO.SetStencilTest(material.StencilTest());
		opaquePSO.SetFrameBuffers(mSwapChain->GetBackBufferDesc().mFormat, mDepthStencilBuffer->GetFormat());	// TODO:
		opaquePSO.SetShader(renderItem.mMaterial->GetShader());
		pRenderContext->SetPipelineState(opaquePSO);

		const MaterialInstance& materialInstance = *renderItem.mMaterial;

		// update and bind constants(uniforms)
		transform.mModel = renderItem.mModel;
		transform.mModelInverse = renderItem.mModelInverse;
		RHIConstantBuffer* pTransformCBuffer = mRenderHardwareInterface->RHIAllocConstantBuffer(sizeof(TransformConstants)).release();
		uint8_t numConstants = materialInstance.NumConstantBuffers() + 1;	// material constants + 1 transform constants
		std::unique_ptr<RHIConstantBuffer*[]> cbuffers = std::make_unique<RHIConstantBuffer*[]>(numConstants);
		cbuffers[0] = pTransformCBuffer;
		mRenderHardwareInterface->UpdateConstantBuffer(pTransformCBuffer, &transform, 0, sizeof(TransformConstants));
		for (uint8_t i = 1; i < numConstants; ++i)
		{
			const Blob& constant = materialInstance.GetConstantBuffer(i - 1);
			cbuffers[i] = mRenderHardwareInterface->RHIAllocConstantBuffer(constant.Size()).release();
			mRenderHardwareInterface->UpdateConstantBuffer(cbuffers[i], constant.Binary(), 0, constant.Size());
		}
		pRenderContext->SetConstantBuffers(1, numConstants, cbuffers.get());
		for (uint8_t i = 0; i < numConstants; ++i)
		{
			delete cbuffers[i];
		}

		// bind textures/instance buffer.
		uint8_t numTextures = materialInstance.NumTextures();
		std::unique_ptr<RHINativeTexture*[]> textures = std::make_unique<RHINativeTexture*[]>(numTextures);
		for (uint8_t i = 0; i < numTextures; ++i)
		{
			textures[i] = static_cast<RHINativeTexture*>(mGPUResources[materialInstance.GetTexture(i)].get());
		}
		pRenderContext->SetTextures(0, numTextures, textures.get());

		// bind vertex buffers and index buffer.
		RHIVertexBuffer* vertexBuffers[] = { static_cast<RHIVertexBuffer*>(mGPUResources[renderItem.mMeshData.mVertexBuffer].get()) };
		pRenderContext->SetVertexBuffers(vertexBuffers, 1);
		pRenderContext->SetIndexBuffer(static_cast<RHIIndexBuffer*>(mGPUResources[renderItem.mMeshData.mIndexBuffer].get()));

		// append draw call
		const SubMesh* subMeshes = renderItem.mMeshData.mSubMeshes.get();
		uint32_t numSubMeshes = renderItem.mMeshData.mSubMeshCount;
		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			// TODO: support gpu instancing.
			pRenderContext->DrawIndexedInstanced(subMeshes[i].mIndexNum, subMeshes[i].mStartIndex, 1);
		}
		pRenderContext->EndDrawCalls();
	}
}

void Renderer::postRender()
{
        
}
