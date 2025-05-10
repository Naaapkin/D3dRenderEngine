#pragma once
#include "Blob.h"

class RHIShader final : public RHIObject
{
public:
    void SetShaders(const Blob& pVertexShader, const Blob& pHullShader, const Blob& pDomainShader, const Blob& pGeometryShader, const Blob& pPixelShader)
    {
        mVertexShader = {pVertexShader.Binary(), pVertexShader.Size()};
        mHullShader = {pHullShader.Binary(), pHullShader.Size()};
        mDomainShader = {pDomainShader.Binary(), pDomainShader.Size()};
        mGeometryShader = {pGeometryShader.Binary(), pGeometryShader.Size()};
        mPixelShader = {pPixelShader.Binary(), pPixelShader.Size()};
    }
    void SetName(const std::string& name) { mName = name; }
    void SetShaderProperties(const std::vector<ShaderProp>& props) { mProps = props; }
    void SetShaderInputs(const std::vector<ShaderInput>& inputs) { mInputs = inputs; }
    const std::vector<ShaderInput>& GetInputElements() const { return mInputs; }
    const std::vector<ShaderProp>& GetShaderProperties() const { return mProps; }
    const std::string& GetName() const { return mName; }
    const Blob& VertexShader() const { return mVertexShader; }
    const Blob& HullShader() const { return mHullShader; }
    const Blob& DomainShader() const { return mDomainShader; }
    const Blob& GeometryShader() const { return mGeometryShader; }
    const Blob& PixelShader() const { return mPixelShader; }
    RHIShader() = default;
    
private:
    Blob mVertexShader;
    Blob mHullShader;
    Blob mDomainShader;
    Blob mGeometryShader;
    Blob mPixelShader;

    std::string mName;
    std::vector<ShaderInput> mInputs;
    std::vector<ShaderProp> mProps;
};
