#include "common/Samplers.hlsl"

static const float2 positions[4] =
{
    float2(-1.0, 1.0), // 左上
    float2(1.0, 1.0), // 右上
    float2(-1.0, -1.0), // 左下
    float2(1.0, -1.0) // 右下
};

static const float2 uvs[4] =
{
    float2(0.0, 0.0), // 左上 UV
    float2(1.0, 0.0), // 右上 UV
    float2(0.0, 1.0), // 左下 UV
    float2(1.0, 1.0) // 右下 UV
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = uvs[vertexID];
    return output;
}

// 常量缓冲区：UI颜色和纹理开关
cbuffer UIConstants : register(b1)
{
    float4 m_color;
    float useTexture;
};

Texture2D uiTexture : register(t1);

float4 PS(VSOutput input) : SV_Target
{
    if (useTexture > 0.5)
    {
        return step(useTexture, 0.5) * uiTexture.Sample(LinearSampler, input.uv) * m_color;
    }
    return m_color; // 直接返回固定颜色
}