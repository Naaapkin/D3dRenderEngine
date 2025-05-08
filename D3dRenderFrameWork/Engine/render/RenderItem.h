#pragma once
#include "BuiltinShaderDef.h"
#include "Engine/pch.h"
#include "Engine/render/MeshData.h"
#include "Engine/render/Material.h"

// only support forward render path currently
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
    SKYBOX_PROCEDURAL
};

struct RenderItem
{
    Matrix4x4 mModel = DirectX::XMMatrixIdentity();   // TODO: Support sub mesh
    Matrix4x4 mModelInverse = DirectX::XMMatrixIdentity();
    MeshData mMeshData{};
    MaterialInstance* mMaterial = nullptr;
};

struct RenderList final
{
    //FrameBufferRef mFrameBuffer;  // nullptr to force rendering to screen.
    CameraConstants mCameraConstants;
    RHIShader* mSkyboxShader;
    SkyboxType mSkyBoxType;
    uint8_t mStencilValue;
    Float4 mBackGroundColor;
    std::vector<RenderItem> mOpaqueList;
    std::vector<RenderItem> mTransparentList;
    std::vector<RenderItem> mUIElements;
};
