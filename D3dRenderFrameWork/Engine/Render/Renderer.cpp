#include "Renderer.h"

#include "DynamicRHI.h"

std::unique_ptr<Material> Renderer::createMaterial(ShaderRef shader)
{
	ASSERT(shader.mObject, TEXT("shader must not be null."));
	uint32_t numCBuffers = 0;
	uint32_t numResources = 0;
	uint32_t numSamplers = 0;
	const std::vector<ShaderProp>& properties = shader->GetShaderProperties();
	for (auto& shaderProp : properties)
	{
		switch (shaderProp.mType)
		{
		case ShaderPropType::CBUFFER:
			if (shaderProp.mRegister <= OBJECT_CONSTANT_REGISTER) break;
			numCBuffers++;
			break;
		case ShaderPropType::TBUFFER:
		case ShaderPropType::TEXTURE:
			numResources++;
			break;
		case ShaderPropType::SAMPLER:
			numSamplers++;
			break;
		}
	}
	Material* material = new Material();
	material->mNumConstants = numCBuffers;
	material->mNumTextures = numResources;
	material->mNumSamplers = numSamplers;
	material->mConstants.reset(new ConstantProperty[numCBuffers]);
	material->mResources.reset(new ResourceProperty[numResources]);
	material->mSamplers.reset(new SamplerProperty[numSamplers]);
	material->mShader = shader;
	numCBuffers = 0;
	numResources = 0;
	numSamplers = 0;
	for (uint32_t i = 0; i < properties.size(); ++i)
	{
		const ShaderProp& prop = properties[i];
#ifdef UNICODE
		std::wstring&& propName = ::AsciiToUtf8(prop.mName);
#else
		std::string& propName = prop.mName;
#endif
		switch (prop.mType)
		{
		case ShaderPropType::CBUFFER:
			if (prop.mRegister <= OBJECT_CONSTANT_REGISTER) break;
			material->mConstants[numCBuffers] = ConstantProperty{ propName, prop.mRegister, prop.mInfo.mCBufferSize };
			numCBuffers++;
			break;
		case ShaderPropType::TBUFFER:
		case ShaderPropType::TEXTURE:
			material->mResources[numResources] = ResourceProperty{ propName, prop.mRegister, prop.mInfo.mDimension };
			numResources++;
			break;
		case ShaderPropType::SAMPLER:
			//material->mSamplers[numSamplers] = { propName };
			numSamplers++;
			break;
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
	initialize(RenderConfiguration{});
}

void Renderer::initialize(const RenderConfiguration& configuration)
{
	mPassConstants.reset(new PassConstants{});

	// initialize legacyRender hardware interface(rhi).
	mRenderHardwareInterface = GetRHI();
	mRenderHardwareInterface->Initialize();

	RHISwapChainDesc swapChainDesc;
	swapChainDesc.mFormat = configuration.mBackBufferFormat;
	swapChainDesc.mWidth = 0;
	swapChainDesc.mHeight = 0;
	swapChainDesc.mNumBackBuffers = configuration.mNumBackBuffers;
	swapChainDesc.mIsFullScreen = false;
	swapChainDesc.mMSAA = 1;
	swapChainDesc.mTargetFramesPerSec = 0;   // use zero to force the native display's refresh rate. 
	mSwapChain = mRenderHardwareInterface->RHICreateSwapChain(swapChainDesc);

	// Create depth-stencil buffer
	RHITextureDesc depthTextureDesc = mSwapChain->GetBackBufferDesc();
	depthTextureDesc.mFormat = configuration.mDepthStencilFormat;
	mDepthStencilBuffer = mRenderHardwareInterface->RHIAllocDepthStencil(depthTextureDesc).release();
	mGPUResources.emplace_back(mDepthStencilBuffer);

	// Create legacyRender contexts, number of legacyRender contexts must not be larger than back buffer count.
	mNumRenderContexts = configuration.mNumBackBuffers;
	mCurrentRenderContextIndex = 0;
	RHIGraphicsContext* pRenderContext;
	mRenderContexts.reset(new RenderContext[configuration.mNumBackBuffers]);
	for (uint8_t i = 0; i < configuration.mNumBackBuffers; ++i)
	{
		mRenderHardwareInterface->RHICreateGraphicsContext(&pRenderContext);
		mRenderContexts[i].mGraphicContext = std::unique_ptr<RHIGraphicsContext>(pRenderContext);
		mRenderContexts[i].mFenceGPU = mRenderHardwareInterface->RHICreateFence();
		mRenderContexts[i].mFenceCPU = 0;
	}

	RHICopyContext* pCopyContext;
	mRenderHardwareInterface->RHICreateCopyContext(&pCopyContext);
	mCopyContext.reset(pCopyContext);
	mCopyFenceGPU = mRenderHardwareInterface->RHICreateFence();
	mCopyFenceCPU = 0;

	//createBuiltinResources();

	// prepare pipeline states
	mPipeStateInitializers.resize(NUM_PRESETS);
	if (configuration.mEnablePreDepth) tryEnablePreDepth();

	PipelineInitializer& skybox = mPipeStateInitializers[PSO_SKY_BOX];
	skybox = PipelineInitializer::Default();
	skybox.SetBlend(false, false, BlendDesc::Disabled());
	DepthTestDesc&& depthTest = DepthTestDesc::Default();
	depthTest.mDepthWriteMask = 0;
	skybox.SetDepthTest(depthTest);
	skybox.SetCullMode(CullMode::FRONT);
	skybox.SetStencilTest(StencilTestDesc::Disabled());

	PipelineInitializer& shadow = mPipeStateInitializers[PSO_SHADOW];
	shadow = PipelineInitializer::Default();

	PipelineInitializer& opaque = mPipeStateInitializers[PSO_OPAQUE];
	opaque = PipelineInitializer::Default();
	opaque.SetBlend(false, false, BlendDesc::Disabled());

	PipelineInitializer& transparent = mPipeStateInitializers[PSO_TRANSPARENT];
	transparent = PipelineInitializer::Default();

	uint16_t quadIndices[6] = { 0, 1, 2, 0, 2, 3 };
	sQuadMeshIndexBuffer = allocIndexBuffer(6, Format::R16_UINT);
	updateIndexBuffer(quadIndices, sizeof(uint16_t) * 6, sQuadMeshIndexBuffer, true);
}

void Renderer::enablePreDepth(std::unique_ptr<RHIShader> preDepthShader)
{
	if (!mPreDepthEnabled && preDepthShader)
	{
		mShaderRegisterMutex.lock();
		if (mAvailableShaderId.empty())
		{
			sPreDepthShader.mIndex = mShaders.size();
			sPreDepthShader.mObject = preDepthShader.get();
			mShaders.emplace_back(preDepthShader.release());
		}
		else
		{
			sPreDepthShader.mIndex = mAvailableShaderId.top();
			mAvailableShaderId.pop();
			sPreDepthShader.mObject = preDepthShader.get();
			mShaders[sPreDepthShader.mIndex].reset(preDepthShader.release());
		}
		mShaderRegisterMutex.unlock();
		tryEnablePreDepth();
	}
}

void Renderer::disablePreDepth()
{
	mPreDepthEnabled = false;
}

Float2 Renderer::getResolution() const
{
	if (!mSwapChain) return {};
	const RHITextureDesc& desc = mSwapChain->GetBackBufferDesc();
	return { static_cast<float>(desc.mWidth), static_cast<float>(desc.mHeight) };
}

void Renderer::terminate()
{
	mRenderContexts[mCurrentRenderContextIndex].mFenceGPU->Wait(mRenderContexts[mCurrentRenderContextIndex].mFenceCPU);
	mCopyFenceGPU->Wait(mCopyFenceCPU);

	for (int i = 0; i < mNumRenderContexts; ++i)
	{
		RenderContext& renderContext = mRenderContexts[i];
		renderContext.mFenceGPU->Wait(renderContext.mFenceCPU);
		for (RHIConstantBuffer* mReleasingCBuffer : renderContext.mReleasingCBuffers)
		{
			mRenderHardwareInterface->RHIReleaseConstantBuffer(mReleasingCBuffer);
		}
	}
	mCopyContext.reset();

#ifdef ENABLE_LEGACY_RENDER_LOOP
	mRenderLists.clear();
#endif
	mRenderQueues.clear();
	mPassConstants.reset();
	mShaderRegisterMutex.lock();
	mShaders.clear();
	mShaderRegisterMutex.unlock();
	mGPUResources.clear();

	mSwapChain.reset();
	mRenderHardwareInterface->Release();
}

VertexBufferRef Renderer::allocVertexBuffer(uint32_t numVertices, uint32_t vertexSize)
{
	RHIVertexBuffer* pVertexBuffer = mRenderHardwareInterface->RHIAllocVertexBuffer(vertexSize, numVertices).release();
	return { allocGPUResource(pVertexBuffer), pVertexBuffer };
}

IndexBufferRef Renderer::allocIndexBuffer(uint32_t numIndices, Format indexFormat)
{
	RHIIndexBuffer* pIndexBuffer = mRenderHardwareInterface->RHIAllocIndexBuffer(numIndices, indexFormat).release();
	return { allocGPUResource(pIndexBuffer), pIndexBuffer };
}

TextureRef Renderer::allocTexture2D(Format format, uint32_t width, uint32_t height, uint8_t mipLevels)
{
	RHINativeTexture* pTexture = mRenderHardwareInterface->RHIAllocTexture({ format, ResourceDimension::TEXTURE2D, width, height, 1, mipLevels, 1, 0 }).release();
	return { allocGPUResource(pTexture), pTexture };
}

void Renderer::updateVertexBuffer(const void* pData, uint64_t bufferSize, VertexBufferRef vertexBufferGPU, bool blockRendering)
{
	uint64_t size = std::min<uint64_t>(vertexBufferGPU->GetBuffer()->BufferSize(), bufferSize);
	auto&& stagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(size);
	mRenderHardwareInterface->RHIUpdateStagingBuffer(stagingBuffer.get(), pData, 0, size);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->RHIResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateBuffer(vertexBufferGPU.mObject, stagingBuffer.get(), size, 0, 0);
	mRenderHardwareInterface->RHISubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->RHISyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

void Renderer::updateIndexBuffer(const void* pData, uint64_t bufferSize, IndexBufferRef indexBufferGPU, bool blockRendering)
{
	uint64_t size = std::min<uint64_t>(indexBufferGPU->GetBuffer()->BufferSize(), bufferSize);
	auto&& stagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(size);
	mRenderHardwareInterface->RHIUpdateStagingBuffer(stagingBuffer.get(), pData, 0, size);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->RHIResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateBuffer(indexBufferGPU.mObject, stagingBuffer.get(), size, 0, 0);
	mRenderHardwareInterface->RHISubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->RHISyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

void Renderer::updateTexture(const void* pData, TextureRef textureGPU, uint8_t mipmap, bool blockRendering)
{
	const RHITextureDesc& desc = textureGPU->GetDesc();
	// TODO: support update multi-mips
	std::unique_ptr<RHIStagingBuffer> pStagingBuffer = mRenderHardwareInterface->RHIAllocStagingBuffer(desc, mipmap);
	mRenderHardwareInterface->RHIUpdateStagingBuffer(pStagingBuffer.get(), desc, pData, mipmap);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->RHIResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateTexture(textureGPU.mObject, pStagingBuffer.get(), 0);
	mRenderHardwareInterface->RHISubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->RHISyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

	if (!blockRendering) return;;
	mRenderContexts[mCurrentRenderContextIndex].mGraphicContext->InsertFence(mCopyFenceGPU.get(), mCopyFenceCPU);
}

void Renderer::render()
{
	RenderContext& renderContext = mRenderContexts[mCurrentRenderContextIndex];

	neoBeginFrame(renderContext);

	RHIGraphicsContext* pGraphicsContext = renderContext.mGraphicContext.get();
	renderContext.mFenceGPU->Wait(renderContext.mFenceCPU);
	mRenderHardwareInterface->RHIResetGraphicsContext(pGraphicsContext);
	mSwapChain->BeginFrame(pGraphicsContext);

	for (const NeoRenderQueue& renderQueue : mRenderQueues)
	{
		RHIRenderTarget* pRenderTarget = renderQueue.mRenderTarget.mObject;
		RHIDepthStencil* pDepthStencil = renderQueue.mDepthStencil.mObject;
		Viewport& viewport = const_cast<Viewport&>(renderQueue.mBaseLayer.mCameraInfo.mViewport);
		Rect& scissorRect = const_cast<Rect&>(renderQueue.mBaseLayer.mCameraInfo.mScissorRect);
		const NeoRenderLayer& layer = renderQueue.mBaseLayer;
		const RHITextureDesc& rtDesc = pRenderTarget->GetBuffer()->GetDesc();
		const RHITextureDesc& dsDesc = pDepthStencil->GetBuffer()->GetDesc();

		mPipeStateInitializers[PSO_SKY_BOX].SetFrameBuffers(rtDesc.mFormat, dsDesc.mFormat);
		mPipeStateInitializers[PSO_PRE_DEPTH].SetFrameBuffers(Format::UNKNOWN, dsDesc.mFormat);
		mPipeStateInitializers[PSO_OPAQUE].SetFrameBuffers(rtDesc.mFormat, dsDesc.mFormat);

		pGraphicsContext->SetViewPorts(&viewport, 1);
		pGraphicsContext->SetScissorRect(&scissorRect, 1);
		std::unique_ptr<RHIConstantBuffer> pViewConstantBuffer;
		// TODO:
		{
			pViewConstantBuffer.reset(pGraphicsContext->AllocTempConstantBuffer(sizeof(NeoCameraConstants)).release()); //  TODO:
			NeoCameraConstants cameraConstants{};
			cameraConstants.mView = layer.mCameraInfo.mView;
			cameraConstants.mViewInverse = DirectX::XMMatrixInverse(nullptr, layer.mCameraInfo.mView);
			cameraConstants.mProjection = layer.mCameraInfo.mProj;
			cameraConstants.mProjectionInverse = DirectX::XMMatrixInverse(nullptr, layer.mCameraInfo.mProj);
			cameraConstants.mViewport = { viewport.mWidth, viewport.mHeight };
			cameraConstants.mClips = layer.mCameraInfo.mClips;
			pGraphicsContext->UpdateConstantBuffer(pViewConstantBuffer.get(), &cameraConstants, 0, sizeof(NeoCameraConstants));
		}
		pGraphicsContext->SetConstantBuffer(VIEW_CONSTANT_REGISTER, pViewConstantBuffer.get());

		if (mPreDepthEnabled)
		{
			pGraphicsContext->SetRenderTargetsAndDepthStencil(nullptr, 0, pDepthStencil);
			pGraphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, renderQueue.mStencilValue, &scissorRect, 1);
			neoPreDepthPass(*pGraphicsContext, layer.mOpaqueBatches, layer.mNumOpaqueBatches);

			pGraphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
			pGraphicsContext->ClearRenderTarget(pRenderTarget, renderQueue.mBackGroundColor, &scissorRect, 1);
		}
		else
		{
			pGraphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
			pGraphicsContext->ClearRenderTarget(pRenderTarget, renderQueue.mBackGroundColor, &scissorRect, 1);
			pGraphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, renderQueue.mStencilValue, &scissorRect, 1);
		}

		neoSkyboxPass(*pGraphicsContext, renderQueue.mSkyboxType, renderQueue.mSkyboxMaterial);
		neoOpaquePass(*pGraphicsContext, layer.mOpaqueBatches, layer.mNumOpaqueBatches);

		for (uint32_t i = 0; i < renderQueue.mNumOverlays; ++i)
		{
			const NeoRenderLayer& overlay = renderQueue.mOverlays[i];
			neoOpaquePass(*pGraphicsContext, overlay.mOpaqueBatches, overlay.mNumOpaqueBatches);
		}
	}
	mSwapChain->EndFrame(pGraphicsContext);
	mRenderHardwareInterface->RHISubmitRenderCommands(pGraphicsContext);
	mRenderHardwareInterface->RHISyncGraphicContext(renderContext.mFenceGPU.get(), ++renderContext.mFenceCPU);
	mSwapChain->Present();
	mCurrentRenderContextIndex = (mCurrentRenderContextIndex + 1) % mNumRenderContexts;

	mRenderQueues.clear();
}

void Renderer::neoPreDepthPass(RHIGraphicsContext& graphicsContext, const NeoRenderBatch* renderBatches, uint32_t numBatches)
{
	PipelineInitializer& preDepthPSO = mPipeStateInitializers[PSO_PRE_DEPTH];
	for (uint32_t i = 0; i < numBatches; ++i)
	{
		const NeoRenderBatch& renderBatch = renderBatches[i];
		const MaterialInstance& material = *renderBatch.mMaterial;
		const DepthTestDesc& depthTest = material.DepthTest();
		if (!depthTest.mEnableDepthTest) continue;
		preDepthPSO.SetCullMode(material.GetCullMode());
		preDepthPSO.SetDrawMode(material.GetDrawMode());
		preDepthPSO.SetDepthTest(depthTest);
		preDepthPSO.SetStencilTest(material.StencilTest());
		graphicsContext.SetPipelineState(preDepthPSO);

		for (uint32_t j = 0; j < renderBatch.mNumRenderItems; ++j)
		{
			const NeoRenderItem& renderItem = renderBatch.mRenderItems[i];
			// instance draw was not supported in pre-depth
			if (renderItem.mIsInstanced) continue;
			const MeshData& meshData = *renderItem.mMeshData;
			RHIVertexBuffer* pVertexBuffer = meshData.mVertexBuffer.mObject;
			RHIIndexBuffer* pIndexBuffer = meshData.mIndexBuffer.mObject;
			SubMesh* subMeshes = meshData.mSubMeshes.get();

			graphicsContext.BeginBinding();

			std::unique_ptr<RHIConstantBuffer> pTransformBuffer = graphicsContext.AllocTempConstantBuffer(renderItem.mInstanceDataStride);
			graphicsContext.UpdateConstantBuffer(pTransformBuffer.get(), renderItem.mInstanceData, 0, renderItem.mInstanceDataStride);
			graphicsContext.SetConstantBuffer(OBJECT_CONSTANT_REGISTER, pTransformBuffer.get());

			graphicsContext.SetVertexBuffers(&pVertexBuffer, 1);
			graphicsContext.SetIndexBuffer(pIndexBuffer);
			graphicsContext.EndBinding();

			for (uint8_t k = 0; k < meshData.mSubMeshCount; ++k)
			{
				graphicsContext.DrawIndexedInstanced(subMeshes[k].mIndexNum, subMeshes[k].mStartIndex, subMeshes[k].mBaseVertex, renderItem.mNumInstance, 0);
			}
		}
	}
}

void Renderer::neoSkyboxPass(RHIGraphicsContext& graphicsContext, SkyboxType type, const MaterialInstance* pMaterial)
{
	if (type == SkyboxType::NONE) return;

	PipelineInitializer& skyboxPSO = mPipeStateInitializers[PSO_SKY_BOX];
	std::unique_ptr<RHIConstantBuffer> pSkyboxConstantBuffer;
	{
		pSkyboxConstantBuffer.reset(graphicsContext.AllocTempConstantBuffer(sizeof(LightConstant)).release()); //  TODO:
		graphicsContext.UpdateConstantBuffer(pSkyboxConstantBuffer.get(), &mPassConstants->mLightConstant, 0, sizeof(LightConstant));
	}
	graphicsContext.SetConstantBuffer(PASS_CONSTANT_REGISTER, pSkyboxConstantBuffer.get());
	
	if (type == SkyboxType::SKYBOX_PROCEDURAL)
	{
		skyboxPSO.SetShader(pMaterial->GetShader().mObject);
		skyboxPSO.SetCullMode(CullMode::BACK);
		graphicsContext.SetPipelineState(skyboxPSO);

		graphicsContext.BeginBinding();
		uint8_t numConstantBuffers = pMaterial->NumConstantBuffers();
		for (uint8_t i = 0; i < numConstantBuffers; ++i)
		{
			graphicsContext.SetConstantBuffer(pMaterial->mMaterial->mConstants[i].mRegisterSlot, pMaterial->mConstantsGPU[i].mObject);
		}
		graphicsContext.SetVertexBuffers(nullptr, 0);
		graphicsContext.SetIndexBuffer(sQuadMeshIndexBuffer.mObject);
		graphicsContext.EndBinding();

		graphicsContext.DrawIndexedInstanced(6, 0, 0, 1, 0);
	}
	else
	{
		WARN("unsupported skybox type.");
	}
}

void Renderer::neoOpaquePass(RHIGraphicsContext& graphicsContext, const NeoRenderBatch* renderBatches, uint32_t numBatches)
{
	PipelineInitializer& opaquePSO = mPipeStateInitializers[PSO_OPAQUE];

	std::unique_ptr<RHIConstantBuffer> pOpaqueConstantBuffer;
	{
		pOpaqueConstantBuffer.reset(graphicsContext.AllocTempConstantBuffer(sizeof(LightConstant)).release()); //  TODO:
		graphicsContext.UpdateConstantBuffer(pOpaqueConstantBuffer.get(), &mPassConstants->mLightConstant, 0, sizeof(LightConstant));
	}
	graphicsContext.SetConstantBuffer(PASS_CONSTANT_REGISTER, pOpaqueConstantBuffer.get());

	for (uint32_t i = 0; i < numBatches; ++i)
	{
		const NeoRenderBatch& renderBatch = renderBatches[i];
		const MaterialInstance& material = *renderBatch.mMaterial;
		opaquePSO.SetShader(material.GetShader().mObject);
		opaquePSO.SetCullMode(material.GetCullMode());
		opaquePSO.SetDrawMode(material.GetDrawMode());
		opaquePSO.SetDepthTest(material.DepthTest());
		opaquePSO.SetStencilTest(material.StencilTest());
		graphicsContext.SetPipelineState(opaquePSO);

		for (uint32_t j = 0; j < renderBatch.mNumRenderItems; ++j)
		{
			const NeoRenderItem& renderItem = renderBatch.mRenderItems[j];
			const MeshData& meshData = *renderItem.mMeshData;
			RHIVertexBuffer* pVertexBuffer = meshData.mVertexBuffer.mObject;
			RHIIndexBuffer* pIndexBuffer = meshData.mIndexBuffer.mObject;
			SubMesh* subMeshes = meshData.mSubMeshes.get();

			graphicsContext.BeginBinding();
			if (renderItem.mIsInstanced)
			{
				std::unique_ptr<RHIInstanceBuffer> pInstanceBuffer = graphicsContext.AllocTempInstanceBuffer(renderItem.mInstanceDataStride * renderItem.mNumInstance,
				                                                          renderItem.mInstanceDataStride);
				graphicsContext.UpdateInstanceBuffer(pInstanceBuffer.get(), renderItem.mInstanceData, 0, renderItem.mInstanceDataStride * renderItem.mNumInstance);
				graphicsContext.SetInstanceBuffer(0, pInstanceBuffer.get());	// TODO:
			}
			else
			{
				std::unique_ptr<RHIConstantBuffer> pTransformBuffer = graphicsContext.AllocTempConstantBuffer(renderItem.mInstanceDataStride);
				graphicsContext.UpdateConstantBuffer(pTransformBuffer.get(), renderItem.mInstanceData, 0, renderItem.mInstanceDataStride);
				graphicsContext.SetConstantBuffer(OBJECT_CONSTANT_REGISTER, pTransformBuffer.get());
			}
			uint8_t numConstantBuffers = material.NumConstantBuffers();
			for (uint8_t k = 0; k < numConstantBuffers; ++k)
			{
				graphicsContext.SetConstantBuffer(material.mMaterial->mConstants[i].mRegisterSlot, material.mConstantsGPU[i].mObject);
			}
			graphicsContext.SetVertexBuffers(&pVertexBuffer, 1);
			graphicsContext.SetIndexBuffer(pIndexBuffer);
			graphicsContext.EndBinding();

			for (uint8_t k = 0; k < meshData.mSubMeshCount; ++k)
			{
				graphicsContext.DrawIndexedInstanced(subMeshes[k].mIndexNum, subMeshes[k].mStartIndex, subMeshes[k].mBaseVertex, renderItem.mNumInstance, 0);
			}
		}
	}
}

std::unique_ptr<RHIShader> Renderer::compileShader(const Blob& blob, ShaderType shaderTypes, const std::wstring* path) const
{
	if (path)
	{
		const std::string pathStr = ::Utf8ToAscii(*path);
		return mRenderHardwareInterface->RHICompileShader(blob, shaderTypes, &pathStr);
	}
	return mRenderHardwareInterface->RHICompileShader(blob, shaderTypes);
}

std::unique_ptr<RHIShader> Renderer::compileShader(const Blob& blob, ShaderType shaderTypes,
	const std::string* path) const
{
	return mRenderHardwareInterface->RHICompileShader(blob, shaderTypes, path);
}

ShaderRef Renderer::compileAndRegisterShader(const std::wstring& shaderName, const Blob& blob, ShaderType shaderTypes,
                                             const std::wstring* path)
{
	const std::string& shaderNameA = ::Utf8ToAscii(shaderName);
	return registerShader(shaderNameA, compileShader(blob, shaderTypes, path));
}

ShaderRef Renderer::compileAndRegisterShader(const std::string& shaderName, const Blob& blob, ShaderType shaderTypes,
	const std::string* path)
{
	return registerShader(shaderName, compileShader(blob, shaderTypes, path));
}

ShaderRef Renderer::registerShader(const std::string& name, std::unique_ptr<RHIShader>&& shader)
{
	shader->SetName(name);
	ShaderRef shaderRef{ 0, shader.release() };
	mShaderRegisterMutex.lock();
	if (!mAvailableShaderId.empty())
	{
		shaderRef.mIndex = mAvailableGPUResourceIds.top();
		mShaders[mAvailableShaderId.top()].reset(shaderRef.mObject);
		mAvailableShaderId.pop();
	}
	else
	{
		shaderRef.mIndex = mShaders.size();
		mShaders.emplace_back(shaderRef.mObject);
	}
	mShaderRegisterMutex.unlock();
	return shaderRef;
}

void Renderer::releaseShader(ShaderRef shader)
{
	if (shader.mObject || shader.mIndex == MAXUINT64)
	{
		WARN("releasing invalid shader reference.");
		return;
	}
	std::unique_ptr<RHIShader>& target = mShaders[shader.mIndex];
	if (target)
	{
		mShaderRegisterMutex.lock();
		mAvailableGPUResourceIds.push(shader.mIndex);
		target.reset();
		mShaderRegisterMutex.unlock();
	}
}

void Renderer::appendRenderQueue(NeoRenderQueue* renderQueues, uint32_t numRenderQueues)
{
	mRenderQueues.reserve(mRenderQueues.size() + numRenderQueues);
	mRenderQueues.insert(mRenderQueues.end(), renderQueues, renderQueues + numRenderQueues);
}

#ifdef ENABLE_LEGACY_RENDER_LOOP
void Renderer::appendRenderLists(std::unique_ptr<RenderList[]> renderLists, uint32_t numRenderLists)
{
	mRenderLists.reserve(mRenderLists.size() + numRenderLists);
	mRenderLists.insert(mRenderLists.end(), renderLists.get(), renderLists.get() + numRenderLists);
}
#endif

void Renderer::setLightConstants(const LightConstant& lightConstants) const
{
	mPassConstants->mLightConstant = lightConstants;
}

void Renderer::setTime(float deltaTime, float time) const
{
	if (mPassConstants->mTime.y == time) return;
	mPassConstants->mTime = { deltaTime, time, std::sin(time), std::cos(time) };
}

void Renderer::setFogConstants(const FogConstant& fogConstants) const
{
	mPassConstants->mFogConstant = fogConstants;
}

#pragma region LegacyRenderLoop
#ifdef ENABLE_LEGACY_RENDER_LOOP
void Renderer::legacyRender()
{
	RenderContext& renderContext = mRenderContexts[mCurrentRenderContextIndex];
	beginFrame(renderContext);
	RHIGraphicsContext* graphicsContext = renderContext.mGraphicContext.get();
	renderContext.mFenceGPU->Wait(renderContext.mFenceCPU);
	mRenderHardwareInterface->RHIResetGraphicsContext(graphicsContext);
	mSwapChain->BeginFrame(graphicsContext);

	for (auto& renderList : mRenderLists)
	{
		// TODO: support specify the legacyRender target and depth stencil.
		RHIRenderTarget* pRenderTarget = mSwapChain->GetCurrentColorTexture();
		RHIDepthStencil* pDepthStencil = mDepthStencilBuffer;
		const RHITextureDesc& rtDesc = pRenderTarget->GetBuffer()->GetDesc();
		const RHITextureDesc& dsDesc = pDepthStencil->GetBuffer()->GetDesc();
		Viewport viewports[] = { Viewport{static_cast<float>(rtDesc.mWidth), static_cast<float>(rtDesc.mHeight), 0, 1} };
		Rect scissorRects[] = { Rect{0, 0, static_cast<int32_t>(rtDesc.mWidth), static_cast<int32_t>(rtDesc.mHeight)} };
		// TODO: Calculations like this can move to pre-legacyRender phase.

		mPipeStateInitializers[PSO_SKY_BOX].SetFrameBuffers(rtDesc.mFormat, dsDesc.mFormat);
		mPipeStateInitializers[PSO_PRE_DEPTH].SetFrameBuffers(Format::UNKNOWN, dsDesc.mFormat);
		mPipeStateInitializers[PSO_OPAQUE].SetFrameBuffers(rtDesc.mFormat, dsDesc.mFormat);

		graphicsContext->SetViewPorts(viewports, 1);
		graphicsContext->SetScissorRect(scissorRects, 1);
		
		graphicsContext->SetRenderTargetsAndDepthStencil(nullptr, 0, pDepthStencil);
		//graphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
		if (renderList.mClearRenderTarget)
		{
			graphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, 0, nullptr, 0);
		}

		{
			std::unique_ptr<RHIConstantBuffer> pViewConstantBuffer = graphicsContext->AllocTempConstantBuffer(sizeof(NeoCameraConstants)); //  TODO:
			graphicsContext->UpdateConstantBuffer(pViewConstantBuffer.get(), &renderList.mCameraConstants, 0, sizeof(NeoCameraConstants));
			graphicsContext->SetConstantBuffer(VIEW_CONSTANT_REGISTER, pViewConstantBuffer.get());
		}
		depthPrePass(graphicsContext, renderList.mOpaqueList);

		graphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
		if (renderList.mClearRenderTarget)
		{
			graphicsContext->ClearRenderTarget(pRenderTarget, renderList.mBackGroundColor, scissorRects, 1);
		}
		
		neoSkyboxPass(*graphicsContext, renderList.mSkyBoxType, renderList.mSkyboxMaterial);
		opaquePass(graphicsContext, renderList.mOpaqueList);
		//mRenderHardwareInterface->RHISubmitRenderCommands(graphicsContext);

		//mRenderHardwareInterface->RHIResetGraphicsContext(graphicsContext);
		//graphicsContext->SetViewPorts(viewports, 1);
		//graphicsContext->SetScissorRect(scissorRects, 1);
	}
	mSwapChain->EndFrame(graphicsContext);
	mRenderHardwareInterface->RHISubmitRenderCommands(graphicsContext);
	mRenderHardwareInterface->RHISyncGraphicContext(renderContext.mFenceGPU.get(), ++renderContext.mFenceCPU);
	mSwapChain->Present();
	mCurrentRenderContextIndex = (mCurrentRenderContextIndex + 1) % mNumRenderContexts;

	mRenderLists.clear();
}

void Renderer::depthPrePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderList)
{
	PipelineInitializer& preDepthPSO = mPipeStateInitializers[PSO_PRE_DEPTH];
	NeoTransformConstants transform{ DirectX::XMMatrixIdentity(), DirectX::XMMatrixIdentity() };
	for (const auto& renderItem : renderList)
	{
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
		std::unique_ptr<RHIConstantBuffer> cbuffer = pRenderContext->AllocTempConstantBuffer(sizeof(NeoTransformConstants));
		mRenderHardwareInterface->RHIUpdateConstantBuffer(cbuffer.get(), &transform, 0, sizeof(NeoTransformConstants));

		pRenderContext->BeginBinding();
		pRenderContext->SetConstantBuffer(OBJECT_CONSTANT_REGISTER, cbuffer.get());

		RHIVertexBuffer* vertexBuffers[] = { renderItem.mMeshData.mVertexBuffer.mObject };
		pRenderContext->SetVertexBuffers(vertexBuffers, 1);
		pRenderContext->SetIndexBuffer(renderItem.mMeshData.mIndexBuffer.mObject);
		pRenderContext->EndBinding();

		const SubMesh* subMeshes = renderItem.mMeshData.mSubMeshes.get();
		uint32_t numSubMeshes = renderItem.mMeshData.mSubMeshCount;
		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			// TODO: support gpu instancing.
			pRenderContext->DrawIndexedInstanced(subMeshes[i].mIndexNum, subMeshes[i].mStartIndex, subMeshes[i].mBaseVertex, 1, 0);
		}
	}
}

void Renderer::opaquePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderBatches)
{
	PipelineInitializer& opaquePSO = mPipeStateInitializers[PSO_OPAQUE];
	for (const auto& renderItem : renderBatches)
	{
		// set pipeline states
		const MaterialInstance& material = *renderItem.mMaterial;
		opaquePSO.SetCullMode(material.GetCullMode());
		opaquePSO.SetDrawMode(material.GetDrawMode());
		opaquePSO.SetDepthTest(material.DepthTest());
		opaquePSO.SetStencilTest(material.StencilTest());
		opaquePSO.SetFrameBuffers(mSwapChain->GetBackBufferDesc().mFormat, mDepthStencilBuffer->GetBuffer()->GetDesc().mFormat);	// TODO:
		opaquePSO.SetShader(renderItem.mMaterial->GetShader().mObject);
		pRenderContext->SetPipelineState(opaquePSO);

		const MaterialInstance& materialInstance = *renderItem.mMaterial;

		// update and bind constants(uniforms)
		NeoTransformConstants transformConstant{ renderItem.mModel , renderItem.mModelInverse };
		std::unique_ptr<RHIConstantBuffer> pTransformCBuffer = pRenderContext->AllocTempConstantBuffer(sizeof(NeoTransformConstants));
		mRenderHardwareInterface->RHIUpdateConstantBuffer(pTransformCBuffer.get(), &transformConstant, 0, sizeof(NeoTransformConstants));

		pRenderContext->BeginBinding();
		pRenderContext->SetConstantBuffer(OBJECT_CONSTANT_REGISTER, pTransformCBuffer.get());
		uint8_t numConstants = materialInstance.NumConstantBuffers();
		for (uint8_t i = 0; i < numConstants; ++i)
		{
			pRenderContext->SetConstantBuffer(material.mMaterial->mConstants[i].mRegisterSlot, materialInstance.mConstantsGPU[i].mObject);
		}
		// bind textures/instance buffer.
		uint8_t numTextures = materialInstance.NumTextures();
		for (uint8_t i = 0; i < numTextures; ++i)
		{
			RHINativeTexture* pTexture = materialInstance.GetTexture(i).mObject;
			if (!pTexture) continue;
			pRenderContext->SetTexture(i, pTexture);
		}

		// bind vertex buffers and index buffer.
		RHIVertexBuffer* vertexBuffers[] = { renderItem.mMeshData.mVertexBuffer.mObject };
		pRenderContext->SetVertexBuffers(vertexBuffers, 1);
		pRenderContext->SetIndexBuffer(renderItem.mMeshData.mIndexBuffer.mObject);
		pRenderContext->EndBinding();

		// append draw call
		const SubMesh* subMeshes = renderItem.mMeshData.mSubMeshes.get();
		uint32_t numSubMeshes = renderItem.mMeshData.mSubMeshCount;
		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			// TODO: support gpu instancing.
			pRenderContext->DrawIndexedInstanced(subMeshes[i].mIndexNum, subMeshes[i].mStartIndex, subMeshes[i].mBaseVertex, 1, 0);
		}
	}
}

#endif
#pragma endregion

Renderer::Renderer() = default;

uint32_t Renderer::allocGPUResource(RHIObject* pObject)
{
	if (mAvailableGPUResourceIds.empty())
	{
		mGPUResources.emplace_back(pObject);
		return mGPUResources.size() - 1;
	}
	uint32_t index = static_cast<uint32_t>(mAvailableGPUResourceIds.top());
	mAvailableGPUResourceIds.pop();
	mGPUResources[index] = std::unique_ptr<RHIObject>(pObject);
	return index;
}

void Renderer::tryEnablePreDepth()
{
	if (!sPreDepthShader.mObject)
	{
		WARN("fail to enable pre-depth pass: cannot find pre-depth shader.")
		return;
	}
	PipelineInitializer& preDepth = mPipeStateInitializers[PSO_PRE_DEPTH];
	preDepth = PipelineInitializer::Default();
	preDepth.SetShader(sPreDepthShader.mObject);
	preDepth.SetBlend(false, false, BlendDesc::Disabled());
	preDepth.SetDepthBias(DepthBiasSet::NORMAL);
	mPreDepthEnabled = true;
}

//void Renderer::createBuiltinResources()
//{
//	const char* builtinShaderSources[] = {
//		"cbuffer ObjectConstants : register(b1) { float4x4 m_model; float4x4 m_model_i; float4x4 m_view; float4x4 m_view_i; float4x4 m_projection; float4x4 m_projection_i; }; struct SimpleVertexInput{float3 position : POSITION;float3 normal : NORMAL;float2 uv : TEXCOORD;};struct FragInput{float4 position : SV_POSITION;};FragInput VsMain(SimpleVertexInput input){FragInput o;float4 worldPosition = mul(m_model, float4(input.position, 1));o.position = mul(m_projection, mul(m_view, worldPosition));return o;}",
//	};
//
//	Blob shaderSource{builtinShaderSources[0], strlen(builtinShaderSources[0])};
//	std::unique_ptr<RHIShader> pShader = compileShader(shaderSource, ShaderType::VERTEX);
//	registerShader(TEXT("PreDepth"), std::move(pShader));
//}
void Renderer::updateMaterialConstants(RenderContext& renderContext, MaterialInstance& materialInstance)
{
	const Material& material = *materialInstance.mMaterial;
	for (int i = 0; i < material.mNumConstants; ++i)
	{
		ConstantBufferRef& cbufferRef = materialInstance.mConstantsGPU[i];
		if (cbufferRef.mObject)
		{
			mGPUResources[cbufferRef.mIndex].release();
			renderContext.mReleasingCBuffers.emplace_back(cbufferRef.mObject);
		}
		else if (mAvailableGPUResourceIds.empty())
		{
			cbufferRef.mIndex = mGPUResources.size();
			cbufferRef.mObject = mRenderHardwareInterface->RHIAllocConstantBuffer(material.mConstants[i].mConstantSize).release();
			mGPUResources.emplace_back(cbufferRef.mObject);
		}
		else
		{
			cbufferRef.mIndex = mAvailableGPUResourceIds.top();
			cbufferRef.mObject = mRenderHardwareInterface->RHIAllocConstantBuffer(material.mConstants[i].mConstantSize).release();
			mGPUResources[mAvailableGPUResourceIds.top()].reset(mRenderHardwareInterface->RHIAllocConstantBuffer(material.mConstants[i].mConstantSize).release());
		}
		const Blob& cbufferCache = materialInstance.GetConstantBuffer(i);
		mRenderHardwareInterface->RHIUpdateConstantBuffer(cbufferRef.mObject, cbufferCache.Binary(), 0, cbufferCache.Size());
	}
	materialInstance.mIsDirty = false;
}

#ifdef ENABLE_LEGACY_RENDER_LOOP

void Renderer::beginFrame(RenderContext& renderContext)
{
	for (RHIConstantBuffer* mReleasingCBuffer : renderContext.mReleasingCBuffers)
	{
		mRenderHardwareInterface->RHIReleaseConstantBuffer(mReleasingCBuffer);
	}

	renderContext.mReleasingCBuffers.clear();

	for (const RenderList& renderList : mRenderLists)
	{
		if (renderList.mSkyBoxType != SkyboxType::NONE && renderList.mSkyboxMaterial->mIsDirty)
		{
			updateMaterialConstants(renderContext, *renderList.mSkyboxMaterial);
		}
		for (const RenderItem& opaqueItem : renderList.mOpaqueList)
		{
			MaterialInstance& materialInstance = *opaqueItem.mMaterial;
			if (!materialInstance.mIsDirty) continue;
			// if the constants of material instance is dirty, reallocate constant buffers for this material.
			updateMaterialConstants(renderContext, materialInstance);
		}
	}
}
#endif

void Renderer::neoBeginFrame(RenderContext& renderContext)
{
	for (RHIConstantBuffer* mReleasingCBuffer : renderContext.mReleasingCBuffers)
	{
		mRenderHardwareInterface->RHIReleaseConstantBuffer(mReleasingCBuffer);
	}
	renderContext.mReleasingCBuffers.clear();

	for (NeoRenderQueue& renderQueue : mRenderQueues)
	{
		renderQueue.mRenderTarget.mObject = renderQueue.mRenderTarget.mObject
			? renderQueue.mRenderTarget.mObject
			: mSwapChain->GetCurrentColorTexture();
		renderQueue.mDepthStencil.mObject = renderQueue.mDepthStencil.mObject
			? renderQueue.mDepthStencil.mObject
			: mDepthStencilBuffer;

		if (renderQueue.mSkyboxType != SkyboxType::NONE && renderQueue.mSkyboxMaterial->mIsDirty)
		{
			updateMaterialConstants(renderContext, *renderQueue.mSkyboxMaterial);
		}

		updateMaterialsInLayer(renderContext, renderQueue.mBaseLayer);
		for (uint32_t i = 0; i < renderQueue.mNumOverlays; ++i)
		{
			updateMaterialsInLayer(renderContext, renderQueue.mOverlays[i]);
		}
	}
}

void Renderer::updateMaterialsInLayer(RenderContext& renderContext, const NeoRenderLayer& layer)
{
	for (uint32_t j = 0; j < layer.mNumOpaqueBatches; ++j)
	{
		const NeoRenderBatch& opaqueBatch = layer.mOpaqueBatches[j];
		for (uint32_t k = 0; k < opaqueBatch.mNumRenderItems; ++k)
		{
			MaterialInstance& materialInstance = *opaqueBatch.mMaterial;
			if (!materialInstance.mIsDirty) continue;
			updateMaterialConstants(renderContext, materialInstance);
		}
	}
}

void Renderer::postRender()
{
        
}

IndexBufferRef Renderer::sQuadMeshIndexBuffer{};
ShaderRef Renderer::sPreDepthShader{ MAXUINT64, nullptr };