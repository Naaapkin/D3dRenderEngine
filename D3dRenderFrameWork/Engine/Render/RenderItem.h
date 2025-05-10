#pragma once
#include "Engine/pch.h"
#include "Material.h"
#include "MeshData.h"
#include "Engine/math/math.h"


// only support forward legacyRender path currently
enum class RenderPass : uint8_t
{
    FORWARD_PRE_DEPTH = 0b1,
    FORWARD_SHADOW = 0b10,
    FORWARD_OPAQUE = 0b100,
    FORWARD_TRANSPARENT = 0b1000,
    FORWARD_POST_TRANSPARENT = 0b10000,
    FORWARD_POST_PROCESS = 0b100000,

    DEFERRED_GEOMETRY = 0b100000,
    DEFERRED_LIGHTING = 0b1000000,
};

enum class SkyboxType
{
    SKYBOX_CUBE_MAP,
    SKYBOX_PANORAMA,
    SKYBOX_PROCEDURAL,
    NONE,
};

struct LightConstant
{
    Float4 mMainLightDir;   // (Direction Light)1 : direction / (Point Light) 2 : position
    Float3 mMainLightColor; // light color (float3) + intensity (float)
    float mMainLightIntensity;
    Float4 mShadowColor;
    Float3 mAmbientLight;   // ambient light (float3) + ambient intensity
    float mAmbientIntensity;
};

struct FogConstant
{
    Float4 mFogColor;   // fogColor.xyz | fog density
    Float4 mFogParams;  // fogStart | fogEnd | preserved | preserved
};

struct NeoCameraConstants
{
    Matrix4x4 mView;
    Matrix4x4 mViewInverse;
    Matrix4x4 mProjection;
    Matrix4x4 mProjectionInverse;
    Float2 mViewport;   // width | height
    Float2 mClips;
};

struct InstanceData
{
    Matrix4x4 mModel;
    Matrix4x4 mModelInverse;
};

struct NeoTransformConstants
{
    Matrix4x4 mModel;
    Matrix4x4 mModelInverse;
};

#ifdef ENABLE_LEGACY_RENDER_LOOP
struct CameraConstants
{
    Matrix4x4 mView;
    Matrix4x4 mViewInverse;
    Matrix4x4 mProj;
    Matrix4x4 mProjectionInverse;
};
#endif

struct CameraInfo
{
    Matrix4x4 mView = Matrix4x4::Identity();
    Matrix4x4 mProj = Matrix4x4::Identity();
    Viewport mViewport{};
    Rect mScissorRect{};
    Float2 mClips;  // near / far
};

struct NeoRenderItem
{
    MeshData* mMeshData = nullptr;
    void* mInstanceData = nullptr;    // TODO: Support sub mesh
    uint32_t mInstanceDataStride = 0;
    uint32_t mNumInstance = 0;
    bool mIsInstanced = false;
};

struct NeoRenderBatch
{
    std::unique_ptr<NeoRenderItem[]> mRenderItems{};
    uint32_t mNumRenderItems = 0;
    MaterialInstance* mMaterial = nullptr;
};

struct NeoRenderLayer final
{
    //FrameBufferRef mFrameBuffer;  // nullptr to force rendering to screen.

    CameraInfo mCameraInfo{};
    NeoRenderBatch* mOpaqueBatches = nullptr;
    NeoRenderBatch* mTransparentBatches = nullptr;
    uint32_t mNumOpaqueBatches = 0;
    uint32_t mNumTransparentBatches = 0;
};

struct NeoRenderQueue final
{
    RenderTargetRef mRenderTarget;  // use RenderTargetRef::NullRef to force back buffer.
    DepthStencilRef mDepthStencil;  // use DepthStencilRef::NullRef to force back buffer matched depth stencil.
    MaterialInstance* mSkyboxMaterial = nullptr;
    Float4 mBackGroundColor{};
    NeoRenderLayer mBaseLayer{};
    NeoRenderLayer* mOverlays = nullptr;
    uint32_t mNumOverlays = 0;
    uint8_t mStencilValue = 0;
    SkyboxType mSkyboxType = SkyboxType::NONE;
};

#ifdef ENABLE_LEGACY_RENDER_LOOP
struct RenderItem
{
	Matrix4x4 mModel = Matrix4x4::Identity();
	Matrix4x4 mModelInverse = Matrix4x4::Identity();
	MeshData mMeshData{};
	MaterialInstance* mMaterial = nullptr;
};

struct RenderList
{
	NeoCameraConstants mCameraConstants{};
	MaterialInstance* mSkyboxMaterial = nullptr;
	Viewport mViewport{};
	Rect mScissorRect{};
	Float4 mBackGroundColor{};
	SkyboxType mSkyBoxType = SkyboxType::SKYBOX_PROCEDURAL;
	uint8_t mStencilValue = 0;
	bool mClearRenderTarget = true;
	std::vector<RenderItem> mOpaqueList{};
};
#endif