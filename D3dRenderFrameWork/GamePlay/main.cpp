#include "Engine/pch.h"
#ifdef RENDER_UNIT_TEST
#include "Engine/common/Exception.h"

#include "Engine/Render/Renderer.h"
#include "Engine/Window/WFrame.h"

namespace Temp
{
    static void LoadShaderFile(const String& path, char** data, uint64_t* size)
    {
        // 1. 打开文件（二进制模式）
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw Exception((TEXT("Failed to open file: ") + path).c_str());
        }

        // 2. 获取文件大小
        std::streampos fileSize = file.tellg();
        if (fileSize == -1) {
            throw Exception((TEXT("Failed to get file Size: ") + path).c_str());
        }
        *size = static_cast<uint64_t>(fileSize);

        // 3. 分配内存（调用方需负责释放！）
        *data = new char[*size];

        // 4. 读取文件内容
        file.seekg(0, std::ios::beg);
        if (!file.read(*data, *size)) {
            delete[] *data; // 读取失败时释放内存
            *data = nullptr;
            *size = 0;
            throw Exception((TEXT("Failed to read file: ") + path).c_str());
        }
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#if defined(DEBUG) or defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
    WFrame& frame = dynamic_cast<WFrame&>(*WFrame::CreateFrame(TEXT("Sample Frame"), 1280, 720));

    // 初始化渲染器
    Renderer& renderer = Renderer::GetInstance();
    renderer.initialize();

    enum TestShaders
    {
        TS_INS_OPAQUE = 0,
	    TS_OPAQUE = 1,
        TS_SKYBOX = 2,
        TS_INS_BLINN_PHONG = 3
    };

    // 加载着色器
    std::vector<String> paths{
        TEXT("Assets/Shaders/source/NeoInstanceOpaque.hlsl"),
        TEXT("Assets/Shaders/source/NeoOpaque.hlsl"),
		TEXT("Assets/Shaders/source/NeoSkybox.hlsl"),
        TEXT("Assets/Shaders/source/NeoInstanceBlinnPhong.hlsl"),
    };
    std::vector<uint8_t> activeShaders{
        ShaderType::VERTEX | ShaderType::PIXEL,
        ShaderType::VERTEX | ShaderType::PIXEL,
        ShaderType::VERTEX | ShaderType::PIXEL,
        ShaderType::VERTEX | ShaderType::PIXEL
    };

    ShaderRef* shaders = new ShaderRef[paths.size()];
    for (uint32_t i = 0; i < activeShaders.size(); i++)
    {
        String& path = paths[i];
        char* data;
        uint64_t size;
        Temp::LoadShaderFile(paths[i], &data, &size);
        Blob shaderSource{ data, size };
        String name = ::GetFileNameFromPath(path);
        shaders[i] = renderer.compileAndRegisterShader(name, shaderSource, static_cast<ShaderType>(activeShaders[i]), &path);
        delete[] data; // 释放内存
    }

    // load pre-depth shader
    if (false)
	{
        String path = TEXT("Assets/Shaders/source/NeoPreDepth.hlsl");
		char* data;
    	uint64_t size;
    	Temp::LoadShaderFile(path, &data, &size);
        Blob shaderSource{ data, size };
        renderer.enablePreDepth(renderer.compileShader(shaderSource, ShaderType::VERTEX, &path));
	}

    // 创建材质
    std::unique_ptr<Material> skyboxMaterial = Renderer::createMaterial(shaders[TS_SKYBOX]);
    std::unique_ptr<MaterialInstance> matInsSkybox = renderer.createMaterialInstance(*skyboxMaterial);

    // 修改材质数据
    uint8_t skyboxConstantID = matInsSkybox->GetCBufferIndex(TEXT("SkyConstants"));
    if (skyboxConstantID != MAXUINT8)
    {
        struct
        {
            float mSkyIntensity = 1.2f;
            float mSunSize = 0.04f;
            float mSunSizeConvergence = 10.0;
            float mAtmosphereThickness = 2.0f;
            Float4 mSkyTint = { 0.6902f, 0.7686f, 0.8706f, 1 };
            Float4 mGroundColor{ 0.3f, 0.3f, 0.4f, 1.0f };
        } skyConstants;
	    matInsSkybox->UpdateConstantBuffer(skyboxConstantID, &skyConstants, sizeof(skyConstants));
    }

    std::unique_ptr<Material> opaqueMaterial = Renderer::createMaterial(shaders[TS_OPAQUE]);
    opaqueMaterial->mCullMode = CullMode::BACK;
    opaqueMaterial->mDrawMode = DrawMode::SOLID;
    opaqueMaterial->mStencilTest = StencilTestDesc::Default();
    opaqueMaterial->mDepthTest = DepthTestDesc::Default();
    opaqueMaterial->mBlend = BlendDesc::Disabled();
	std::unique_ptr<MaterialInstance> matInsOpaque = renderer.createMaterialInstance(*opaqueMaterial);

    std::unique_ptr<Material> opaqueInstanceMaterial = Renderer::createMaterial(shaders[TS_INS_OPAQUE]);
    opaqueInstanceMaterial->mCullMode = CullMode::BACK;
    opaqueInstanceMaterial->mDrawMode = DrawMode::SOLID;
    opaqueInstanceMaterial->mStencilTest = StencilTestDesc::Default();
    opaqueInstanceMaterial->mDepthTest = DepthTestDesc::Default();
    opaqueInstanceMaterial->mBlend = BlendDesc::Disabled();
    std::unique_ptr<MaterialInstance> matInsOpaqueInstance = renderer.createMaterialInstance(*opaqueInstanceMaterial);

    std::unique_ptr<Material> opaqueLitInstanceMaterial = Renderer::createMaterial(shaders[TS_INS_BLINN_PHONG]);
    opaqueLitInstanceMaterial->mCullMode = CullMode::BACK;
    opaqueLitInstanceMaterial->mDrawMode = DrawMode::SOLID;
    opaqueLitInstanceMaterial->mStencilTest = StencilTestDesc::Default();
    opaqueLitInstanceMaterial->mDepthTest = DepthTestDesc::Default();
    opaqueLitInstanceMaterial->mBlend = BlendDesc::Disabled();
    std::unique_ptr<MaterialInstance> matInsOpaqueLitInstance = renderer.createMaterialInstance(*opaqueLitInstanceMaterial);

    // 准备CPU资源
	Mesh cubeMesh = MeshPrototype::CreateCubeMesh();
    const std::vector<float>& vertexBufferCPU = cubeMesh.getFilteredVertexBuffer();
	const std::vector<uint32_t>& indexBufferCPU = cubeMesh.indices();
    byte* cubeTexCPU = new byte[2 * 2 * 4]{255, 255, 255, 255, 255, 255, 255, 255 , 255, 255, 255, 255 , 255, 255, 255, 255};

    // 分配并上传GPU资源
    TextureRef cubeTexGPU = renderer.allocTexture2D(Format::R8G8B8A8_UNORM, 2, 2, 1);
    VertexBufferRef vertexBufferGPU = renderer.allocVertexBuffer(vertexBufferCPU.size() >> 3, sizeof(float[3 + 3 + 2]));
	IndexBufferRef indexBufferGPU = renderer.allocIndexBuffer(indexBufferCPU.size(), Format::R32_UINT);
    // update synchronously
    renderer.updateVertexBuffer(vertexBufferCPU.data(), vertexBufferCPU.size() * sizeof(float), vertexBufferGPU, true);
    renderer.updateIndexBuffer(indexBufferCPU.data(), indexBufferCPU.size() * sizeof(uint32_t), indexBufferGPU, true);
    renderer.updateTexture(cubeTexCPU, cubeTexGPU, 0, true);

    matInsOpaque->SetTexture(0, ResourceDimension::TEXTURE2D, cubeTexGPU);

    LightConstant lightConstants{};
    lightConstants.mAmbientLight = { 0, 0, 0 };
    lightConstants.mAmbientIntensity = 1;
    lightConstants.mMainLightColor = { 0.7f, 0.9f, 0.85f };
    lightConstants.mMainLightDir = { 0, -1, 0, 1 };
    lightConstants.mShadowColor = { 0.2, 0.2, 0.2, 0.2 };
    lightConstants.mMainLightIntensity = 1;

    Blob lightConstantBuffer{ &lightConstants, sizeof(LightConstant) };

    const std::vector<SubMesh> subMeshes = cubeMesh.subMeshes();

#ifdef ENABLE_LEGACY_RENDER_LOOP
    RenderItem renderItem{};
	renderItem.mMaterial = matInsOpaque.get();
	renderItem.mMeshData.mVertexBuffer = vertexBufferGPU;
	renderItem.mMeshData.mIndexBuffer = indexBufferGPU;
    renderItem.mMeshData.mSubMeshCount = 1;
    renderItem.mMeshData.mSubMeshes.reset(new SubMesh[subMeshes.size()]);
    renderItem.mModel = Matrix4x4::Identity();
    renderItem.mModelInverse = Matrix4x4::Identity();
    memcpy(renderItem.mMeshData.mSubMeshes.get(), subMeshes.data(), subMeshes.size() * sizeof(SubMesh));

	RenderList renderList{};
    renderList.mCameraConstants.mView = DirectX::XMMatrixLookAtLH({ 0, -5, -10, 1 },
        { 0, 0, 0, 1 },
        { 0, 1, 0, 0 });
    renderList.mCameraConstants.mViewInverse = DirectX::XMMatrixInverse(nullptr, renderList.mCameraConstants.mView);
    renderList.mCameraConstants.mViewport = renderer.getResolution();

    renderList.mBackGroundColor = { 0.6902f, 0.7686f, 0.8706f, 1.0f };
    renderList.mCameraConstants.mProjection = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 1000.0f, 0.3f);
    renderList.mCameraConstants.mProjectionInverse = DirectX::XMMatrixInverse(nullptr, renderList.mCameraConstants.mProjection);
    renderList.mOpaqueList.push_back(renderItem);
    renderList.mSkyBoxType = SkyboxType::SKYBOX_PROCEDURAL;
    renderList.mSkyboxMaterial = matInsSkybox.get();
#else
    Float2 resolution = renderer.getResolution();
    NeoRenderQueue renderQueue{};
    renderQueue.mRenderTarget = RenderTargetRef::NullRef();
    renderQueue.mDepthStencil = DepthStencilRef::NullRef();
    renderQueue.mNumOverlays = 0;
    renderQueue.mSkyboxMaterial = matInsSkybox.get();
    renderQueue.mSkyboxType = SkyboxType::NONE;
    renderQueue.mBackGroundColor = { 0.6902f, 0.7686f, 0.8706f, 1 };
    renderQueue.mStencilValue = 0;

    NeoRenderLayer& mainCameraRenderLayer = renderQueue.mBaseLayer;
    CameraInfo& cameraInfo = mainCameraRenderLayer.mCameraInfo;
    cameraInfo.mClips = { 0.3f, 1000.0f };
    cameraInfo.mScissorRect = Rect{ 0, 0, static_cast<int32_t>(resolution.x), static_cast<int32_t>(resolution.y) };
    cameraInfo.mViewport = Viewport{ resolution.x, resolution.y, 0, 1 };
    cameraInfo.mProj = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 1000.0f, 0.3f);
    cameraInfo.mView = DirectX::XMMatrixLookAtLH({ 0, 5, -20, 1 },
        { 0, 0, 0, 1 },
        { 0, 1, 0, 0 });

    mainCameraRenderLayer.mNumOpaqueBatches = 1;
    mainCameraRenderLayer.mNumTransparentBatches = 0;
    mainCameraRenderLayer.mTransparentBatches = nullptr;
    mainCameraRenderLayer.mOpaqueBatches = new NeoRenderBatch{};

    NeoRenderBatch& cubeRenderBatch = *mainCameraRenderLayer.mOpaqueBatches;
    cubeRenderBatch.mMaterial = matInsOpaqueLitInstance.get();
    cubeRenderBatch.mNumRenderItems = 1;
    cubeRenderBatch.mRenderItems.reset(new NeoRenderItem{});

#define INSTANCE_COUNT 900
    NeoRenderItem& cubeRI = cubeRenderBatch.mRenderItems[0];
    cubeRI.mIsInstanced = true;
    cubeRI.mMeshData = new MeshData{ vertexBufferGPU, indexBufferGPU, subMeshes.data(), static_cast<uint8_t>(subMeshes.size()) };
    cubeRI.mNumInstance = INSTANCE_COUNT;
    cubeRI.mInstanceData = new NeoTransformConstants[INSTANCE_COUNT];
    cubeRI.mInstanceDataStride = sizeof(NeoTransformConstants);

    NeoTransformConstants* transforms = static_cast<NeoTransformConstants*>(cubeRI.mInstanceData);
    transforms[0].mModel = DirectX::XMMatrixIdentity();
    transforms[0].mModelInverse = DirectX::XMMatrixIdentity();
#endif

    LARGE_INTEGER frequency, start, end, stamp;
    QueryPerformanceFrequency(&frequency);  // 获取计时器频率
    QueryPerformanceCounter(&start);        // 记录起始时间
    stamp = start;

    float deltaTime;

    bool flag = true;
    while (flag)
    {
        MSG msg;
	    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	    {
			if (msg.message == WM_QUIT)
			{
                flag = false;
                break;
			}
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (frame.IsClosed()) PostQuitMessage(0);
	    }

        QueryPerformanceCounter(&end);
        double elapsedTime = static_cast<float>(end.QuadPart - start.QuadPart) / static_cast<float>(frequency.QuadPart);
        deltaTime = static_cast<float>(end.QuadPart - stamp.QuadPart) * 1000.0 / static_cast<float>(frequency.QuadPart);
        stamp = end;
        /*OutputDebugString(std::to_wstring(deltaTime).c_str());
        OutputDebugString(TEXT("\n"));*/

        renderer.setTime(deltaTime, elapsedTime);
        lightConstants.mMainLightDir.z = std::sin(elapsedTime / 4);
        lightConstants.mMainLightDir.y = -std::cos(elapsedTime / 4);
        renderer.setLightConstants(lightConstants);

#ifdef ENABLE_LEGACY_RENDER_LOOP
        //renderList.mOpaqueList[0].mModel = DirectX::XMMatrixRotationY(static_cast<float>(elapsedTime * 0.18));
        //renderList.mOpaqueList[0].mModelInverse = DirectX::XMMatrixInverse(nullptr, renderItem.mModel);

        renderList.mCameraConstants.mView = DirectX::XMMatrixLookAtLH({ 10 * static_cast<float>(std::sin(elapsedTime / 2)), -5, -10 * static_cast<float>(std::cos(elapsedTime / 2)), 1 },
            { 0, 0, 0, 1 },
            { 0, 1, 0, 0 });
        renderList.mCameraConstants.mViewInverse = DirectX::XMMatrixInverse(nullptr, renderList.mCameraConstants.mView);

        std::unique_ptr<RenderList[]> renderLists;
        renderLists.reset(new RenderList[1]{ renderList });
        renderer.appendRenderLists(std::move(renderLists), 1);
        renderer.legacyRender();
#else
        //cameraInfo.mView = DirectX::XMMatrixLookAtLH({ 10 * static_cast<float>(std::sin(elapsedTime / 2)), 5, -10 * static_cast<float>(std::cos(elapsedTime / 2)), 1 },
        //    { 0, 0, 0, 1 },
        //    { 0, 1, 0, 0 });
        using namespace DirectX;

        const int gridSize = 30;
        const float spacing = 1.5f; // 网格中每个方块的间距
        const float frequency = 1.5f;

        for (int i = 0; i < INSTANCE_COUNT; ++i)
        {
	        constexpr float amplitude = 1.0f;
	        int xIndex = i % gridSize;
            int zIndex = i / gridSize;

            float x = (xIndex - gridSize / 2) * spacing;
            float z = (zIndex - gridSize / 2) * spacing;

            float y = std::sin(elapsedTime * frequency + (x + z) * 0.1f) * amplitude;

            transforms[i].mModel = XMMatrixTranslation(x, y, z);
            transforms[i].mModelInverse = XMMatrixInverse(nullptr, transforms[i].mModel);
        }
        renderer.appendRenderQueue(&renderQueue, 1);
        renderer.render();
#endif
    }
    renderer.terminate();

    return 0;
}
#endif