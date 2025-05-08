#include "Renderer.h"

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
	mPassConstants.reset(new PassConstants{});

	// initialize render hardware interface(rhi).
#ifdef WIN32
	mRenderHardwareInterface = new D3D12RHI();
#else
	mRenderHardwareInterface = new PlayStationRHI();
#endif
	RHIConfiguration rhiConfiguration = RHIConfiguration::Default();
	mRenderHardwareInterface->Initialize(rhiConfiguration);
        
	RHISwapChainDesc swapChainDesc;
	swapChainDesc.mFormat = Format::R8G8B8A8_UNORM;
	swapChainDesc.mWidth = 0;
	swapChainDesc.mHeight = 0;
	swapChainDesc.mNumBackBuffers = numBackBuffer;
	swapChainDesc.mIsFullScreen = false;
	swapChainDesc.mMSAA = 1;
	swapChainDesc.mTargetFramesPerSec = 0;   // use zero to force the native display's refresh rate. 
	mSwapChain = mRenderHardwareInterface->RHICreateSwapChain(swapChainDesc);

	// Create depth-stencil buffer
	RHITextureDesc depthTextureDesc = mSwapChain->GetBackBufferDesc(); 
	depthTextureDesc.mFormat = Format::D24_UNORM_S8_UINT;
	mDepthStencilBuffer = mRenderHardwareInterface->RHIAllocDepthStencil(depthTextureDesc).release();
	mGPUResources.emplace_back(mDepthStencilBuffer);

	// Create render contexts, number of render contexts must not be larger than back buffer count.
	mNumRenderContexts = numBackBuffer;
	mCurrentRenderContextIndex = 0;
	RHIGraphicsContext* pRenderContext;
	mRenderContexts.reset(new RenderContext[numBackBuffer]);
	for (uint8_t i = 0; i < numBackBuffer; ++i)
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

	createBuiltinResources();
        
	// prepare pipeline states
	mPipeStateInitializers.resize(NUM_PRESETS);
	PipelineInitializer& preDepth = mPipeStateInitializers[PSO_PRE_DEPTH];
	preDepth = PipelineInitializer::Default();
	static const auto it = mShaderMap.find(TEXT("PreDepth"));
	ASSERT(it != mShaderMap.end(), TEXT("missing mShader : PreDepth"));
	preDepth.SetShader(it->second);
	preDepth.SetBlend(false, false, BlendDesc::Disabled());
	preDepth.SetDepthBias(DepthBiasSet::NORMAL);

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

	uint16_t quadIndices[6] = {0, 1, 2, 0, 2, 3};
	sQuadMeshIndexBuffer = allocIndexBuffer(6, Format::R16_UINT);
	updateIndexBuffer(quadIndices, sizeof(uint16_t) * 6, sQuadMeshIndexBuffer, true);
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
	RHINativeTexture* pTexture = mRenderHardwareInterface->RHIAllocTexture({ format, TextureDimension::TEXTURE2D, width, height, 1, mipLevels, 1, 0 }).release();
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
	std::unique_ptr<RHIStagingBuffer> pStagingBuffer = mRenderHardwareInterface->RHIAllocStagingTexture(desc, mipmap);
	mRenderHardwareInterface->RHIUpdateStagingTexture(pStagingBuffer.get(), desc, pData, mipmap);
	mCopyFenceGPU->Wait(mCopyFenceCPU);
	mRenderHardwareInterface->RHIResetCopyContext(mCopyContext.get());
	mCopyContext->UpdateTexture(textureGPU.mObject, pStagingBuffer.get(), 0);
	mRenderHardwareInterface->RHISubmitCopyCommands(mCopyContext.get());
	mRenderHardwareInterface->RHISyncCopyContext(mCopyFenceGPU.get(), ++mCopyFenceCPU);

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
	mRenderHardwareInterface->RHIResetGraphicsContext(graphicsContext);
	mSwapChain->BeginFrame(graphicsContext);

	for (auto& renderList : mRenderLists)
	{
		// TODO: support specify the render target and depth stencil.
		RHIRenderTarget* pRenderTarget = mSwapChain->GetCurrentColorTexture();
		RHIDepthStencil* pDepthStencil = mDepthStencilBuffer;
		RHITextureDesc rtDesc = pRenderTarget->GetTextureDesc();
		Viewport viewports[] = { Viewport{static_cast<float>(rtDesc.mWidth), static_cast<float>(rtDesc.mHeight), 1, 0} };
		Rect scissorRects[] = { Rect{0, 0, static_cast<int32_t>(rtDesc.mWidth), static_cast<int32_t>(rtDesc.mHeight)} };
		// TODO: Calculations like this can move to pre-render phase.

		mPipeStateInitializers[PSO_SKY_BOX].SetFrameBuffers(pRenderTarget->GetFormat(), pDepthStencil->GetFormat());
		mPipeStateInitializers[PSO_PRE_DEPTH].SetFrameBuffers(Format::UNKNOWN, pDepthStencil->GetFormat());
		mPipeStateInitializers[PSO_OPAQUE].SetFrameBuffers(pRenderTarget->GetFormat(), pDepthStencil->GetFormat());

		mPassConstants->mScreenParams = { viewports[0].mWidth, viewports[0].mHeight, 1, 1 };
		graphicsContext->SetViewPorts(viewports, 1);
		graphicsContext->SetScissorRect(scissorRects, 1);

		graphicsContext->SetRenderTargetsAndDepthStencil(nullptr, 0, pDepthStencil);
		//graphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
		graphicsContext->ClearDepthStencil(pDepthStencil, true, true, 0, 0, nullptr, 0);

		depthPrePass(graphicsContext, renderList.mOpaqueList, renderList.mCameraConstants);

		graphicsContext->SetRenderTargetsAndDepthStencil(&pRenderTarget, 1, pDepthStencil);
		graphicsContext->ClearRenderTarget(pRenderTarget, renderList.mBackGroundColor, scissorRects, 1);
		if (renderList.mSkyboxShader)
		{
			skyboxPass(graphicsContext, *renderList.mSkyboxShader, renderList.mSkyBoxType, renderList.mCameraConstants);
		}
		//mRenderHardwareInterface->RHISubmitRenderCommands(graphicsContext);

		//mRenderHardwareInterface->RHIResetGraphicsContext(graphicsContext);
		//graphicsContext->SetViewPorts(viewports, 1);
		//graphicsContext->SetScissorRect(scissorRects, 1);


		opaquePass(graphicsContext, renderList.mOpaqueList, renderList.mCameraConstants);
	}
	mSwapChain->EndFrame(graphicsContext);
	mRenderHardwareInterface->RHISubmitRenderCommands(graphicsContext);
	mRenderHardwareInterface->RHISyncGraphicContext(renderContext.mFenceGPU.get(), ++renderContext.mFenceCPU);
	mSwapChain->Present();
	mCurrentRenderContextIndex = (mCurrentRenderContextIndex + 1) % mNumRenderContexts;

	mRenderLists.clear();
}

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

void Renderer::createBuiltinResources()
{
	const char* builtinShaderSources[] = {
		"cbuffer ObjectConstants : register(b1) { float4x4 m_model; float4x4 m_model_i; float4x4 m_view; float4x4 m_view_i; float4x4 m_projection; float4x4 m_projection_i; }; struct SimpleVertexInput{float3 position : POSITION;float3 normal : NORMAL;float2 uv : TEXCOORD;};struct FragInput{float4 position : SV_POSITION;};FragInput VsMain(SimpleVertexInput input){FragInput o;float4 worldPosition = mul(m_model, float4(input.position, 1));o.position = mul(m_projection, mul(m_view, worldPosition));return o;}",
	};

	Blob shaderSource{builtinShaderSources[0], strlen(builtinShaderSources[0])};
	std::unique_ptr<RHIShader> pShader = compileShader(shaderSource, ShaderType::VERTEX);
	registerShader(TEXT("PreDepth"), std::move(pShader));
}

void Renderer::beginFrame(RenderContext* pRenderContext)
{
	auto& renderContext = *pRenderContext;
	mRenderHardwareInterface->RHIReleaseConstantBuffers(renderContext.mReleasingCBuffers.data(), renderContext.mReleasingCBuffers.size());
	renderContext.mReleasingCBuffers.clear();
	for (const RenderList& renderList : mRenderLists)
	{
		for (const RenderItem& opaqueItem : renderList.mOpaqueList)
		{
			MaterialInstance& materialInstance = *opaqueItem.mMaterial;
			if (!materialInstance.mIsDirty) continue;
			const Material& material = *materialInstance.mMaterial;
			// if the constants of material instance is dirty, reallocate constant buffers for this material.
			for (int i = 0; i < material.mNumConstants; ++i)
			{
				ConstantBufferRef& cbufferRef = materialInstance.mConstantsGPU[i];
				mGPUResources[cbufferRef.mIndex].release();
				renderContext.mReleasingCBuffers.push_back(cbufferRef.mObject);
				cbufferRef.mObject = mRenderHardwareInterface->RHIAllocConstantBuffer(material.mConstants[i].mConstantSize).release();
				mGPUResources[cbufferRef.mIndex].reset(cbufferRef.mObject);
				const Blob& cbufferCache = materialInstance.GetConstantBuffer(i);
				mRenderHardwareInterface->RHIUpdateConstantBuffer(cbufferRef.mObject, cbufferCache.Binary(), 0, cbufferCache.Size());
			}
		}
	}
	mRenderHardwareInterface->RHIResetGraphicsContext(pRenderContext->mGraphicContext.get());
}

void Renderer::skyboxPass(RHIGraphicsContext* pRenderContext, const RHIShader& skyboxShader,
                          SkyboxType type, const CameraConstants& cameraConstants)
{
	PipelineInitializer& skyboxPSO = mPipeStateInitializers[PSO_SKY_BOX];
	skyboxPSO.SetShader(&skyboxShader);

	struct alignas(256) SkyBoxConstants
	{
		Matrix4x4 mVP;
		Matrix4x4 mVPI;
		Float4 mScreenParams;
	};

	switch (type)
	{
	// only procedural skybox supported
	case SkyboxType::SKYBOX_PROCEDURAL:
		{
		skyboxPSO.SetCullMode(CullMode::BACK);

		pRenderContext->SetPipelineState(skyboxPSO);
		SkyBoxConstants skyboxConstants{ static_cast<DirectX::XMMATRIX>(cameraConstants.mProjection) * cameraConstants.mView ,
			static_cast<DirectX::XMMATRIX>(cameraConstants.mViewInverse) * cameraConstants.mProjectionInverse, mPassConstants->mScreenParams };

		std::unique_ptr<RHIConstantBuffer> cbuffer = pRenderContext->AllocConstantBuffer(sizeof(LightConstant));
		pRenderContext->BeginBinding();
		mRenderHardwareInterface->RHIUpdateConstantBuffer(cbuffer.get(), &mPassConstants->mLightConstant, 0, sizeof(LightConstant));
		pRenderContext->SetConstantBuffer(0, cbuffer.get());

		cbuffer = pRenderContext->AllocConstantBuffer(sizeof(SkyBoxConstants));
		mRenderHardwareInterface->RHIUpdateConstantBuffer(cbuffer.get(), &skyboxConstants, 0, sizeof(SkyBoxConstants));
		pRenderContext->SetConstantBuffer(1, cbuffer.get());
		pRenderContext->SetVertexBuffers(nullptr, 0);
		pRenderContext->SetIndexBuffer(sQuadMeshIndexBuffer.mObject);
		pRenderContext->EndBindings();

		pRenderContext->DrawIndexedInstanced(6, 0, 0, 1, 0);
		}
		break;
	default:
		THROW_EXCEPTION(TEXT("unsupported skybox type."));
		break;
	}
}

void Renderer::depthPrePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants)
{
	PipelineInitializer& preDepthPSO = mPipeStateInitializers[PSO_PRE_DEPTH];
	TransformConstants transform{DirectX::XMMatrixIdentity(), DirectX::XMMatrixIdentity(), cameraConstants };
	for (const auto& renderItem : renderItems)
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
		std::unique_ptr<RHIConstantBuffer> cbuffer = pRenderContext->AllocConstantBuffer(sizeof(TransformConstants));
		mRenderHardwareInterface->RHIUpdateConstantBuffer(cbuffer.get(), &transform, 0, sizeof(TransformConstants));

		pRenderContext->BeginBinding();
		pRenderContext->SetConstantBuffer(1, cbuffer.get());

		RHIVertexBuffer* vertexBuffers[] = { renderItem.mMeshData.mVertexBuffer.mObject };
		pRenderContext->SetVertexBuffers(vertexBuffers, 1);
		pRenderContext->SetIndexBuffer(renderItem.mMeshData.mIndexBuffer.mObject);
		pRenderContext->EndBindings();

		const SubMesh* subMeshes = renderItem.mMeshData.mSubMeshes.get();
		uint32_t numSubMeshes = renderItem.mMeshData.mSubMeshCount;
		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			// TODO: support gpu instancing.
			pRenderContext->DrawIndexedInstanced(subMeshes[i].mIndexNum, subMeshes[i].mStartIndex, subMeshes[i].mBaseVertex, 1, 0);
		}
	}
}

void Renderer::opaquePass(RHIGraphicsContext* pRenderContext, const std::vector<RenderItem>& renderItems, const CameraConstants& cameraConstants)
{
	PipelineInitializer& opaquePSO = mPipeStateInitializers[PSO_OPAQUE];
	TransformConstants transform{ DirectX::XMMatrixIdentity(), DirectX::XMMatrixIdentity(), cameraConstants };
	for (const auto& renderItem : renderItems)
	{
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

		pRenderContext->BeginBinding();
		// update and bind constants(uniforms)
		transform.mModel = renderItem.mModel;
		transform.mModelInverse = renderItem.mModelInverse;
		RHIConstantBuffer* pTransformCBuffer = pRenderContext->AllocConstantBuffer(sizeof(TransformConstants)).release();
		uint8_t numConstants = materialInstance.NumConstantBuffers() + 1;	// material constants + 1 transform constants
		std::unique_ptr<RHIConstantBuffer*[]> cbuffers = std::make_unique<RHIConstantBuffer*[]>(numConstants);
		cbuffers[0] = pTransformCBuffer;
		// TODO: 
		mRenderHardwareInterface->RHIUpdateConstantBuffer(pTransformCBuffer, &transform, 0, sizeof(TransformConstants));
		for (uint8_t i = 1; i < numConstants; ++i)
		{
			const Blob& constant = materialInstance.GetConstantBuffer(i - 1);
			cbuffers[i] = pRenderContext->AllocConstantBuffer(constant.Size()).release();
			mRenderHardwareInterface->RHIUpdateConstantBuffer(cbuffers[i], constant.Binary(), 0, constant.Size());
		}
		pRenderContext->SetConstantBuffers(1, numConstants, cbuffers.get());
		for (uint8_t i = 0; i < numConstants; ++i)
		{
			delete cbuffers[i];
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
		pRenderContext->EndBindings();

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

void Renderer::postRender()
{
        
}

RHIRef<RHIIndexBuffer> Renderer::sQuadMeshIndexBuffer{};