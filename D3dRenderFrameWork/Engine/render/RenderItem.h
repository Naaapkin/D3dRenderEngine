#pragma once
#include "Engine/pch.h"
#include "Engine/render/MeshData.h"
#include "Engine/render/Material.h"

struct alignas(256) TransformConstants
{
    DirectX::XMMATRIX mModel;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};

struct RenderItem
{
    RenderItem() : mModel(DirectX::XMMatrixIdentity()), mMeshData(), mMaterial(nullptr) {}
    RenderItem(DirectX::FXMMATRIX model, MaterialInstance* material, MeshData meshData) : mModel(model), mMeshData(std::move(meshData)), mMaterial(material) {}
    
    DirectX::XMMATRIX mModel;
    MeshData mMeshData;
    MaterialInstance* mMaterial;
};

struct RenderList final
{
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProj;
    std::vector<RenderItem> mRenderItems;
};
