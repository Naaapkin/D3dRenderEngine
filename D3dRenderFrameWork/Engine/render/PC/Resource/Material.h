#pragma once
#include "D3dResource.h"
#include "Shader.h"

class Material
{
public:
    const Shader* shader() const { return mShader; }
    Material(const Shader* pShader) : mShader(pShader) { }
    
private:
    const Shader* mShader;
    std::vector<ResourceHandle> mConstantBuffers;
};
