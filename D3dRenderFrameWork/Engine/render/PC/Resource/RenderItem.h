#pragma once
#include "D3dResource.h"
#include "Engine/pch.h"
#include "Engine/render/MeshData.h"

struct alignas(256) TransformConstants
{
    DirectX::XMMATRIX mModel;
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProjection;
};

class Material
{
public:
    Shader* shader;
    std::vector<std::pair<uint8_t, std::vector<byte>>> mConstants;   // registerId-constant pair
    std::vector<std::pair<uint8_t, ResourceHandle>> mTextures;
};

struct RenderItem
{
    RenderItem() : mModel(DirectX::XMMatrixIdentity()), mMaterial(nullptr) {}
    RenderItem(DirectX::FXMMATRIX model, Material* material, MeshData&& meshData) : mModel(model), mMeshData(std::move(meshData)), mMaterial(material) {}
    
    DirectX::XMMATRIX mModel;
    MeshData mMeshData;
    Material* mMaterial;
};

struct RenderList final
{
    DirectX::XMMATRIX mView;
    DirectX::XMMATRIX mProj;
    std::vector<RenderItem> mRenderItems;
};
