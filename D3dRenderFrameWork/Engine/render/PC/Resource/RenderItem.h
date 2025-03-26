#pragma once
#include "D3dResource.h"
#include "Shader.h"
#include "Engine/pch.h"
#include "Engine/render/MeshData.h"

struct Material
{
    Shader* shader;
    std::vector<ResourceHandle> mConstantBuffers;
};

struct RenderItem
{
    DirectX::XMFLOAT4X4 mModel;
    MeshData mMeshData;
    Material mMaterial;
};

struct RenderList
{
    DirectX::XMFLOAT4X4 mView;
    DirectX::XMFLOAT4X4 mProj;
    std::vector<RenderItem> mRenderItems;
};
