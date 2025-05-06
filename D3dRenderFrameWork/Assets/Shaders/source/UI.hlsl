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

cbuffer UIConstants : register(b1)
{
    float3x3 transform;
    float4 color;
    float blendFactor;
};

VSOutput VS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.position = float4(mul(transform, float3(positions[vertexID], 1)), 1.0);
    output.uv = uvs[vertexID];
    return output;
}

Texture2D uiTexture : register(t1);

float4 PS(VSOutput input) : SV_Target
{
    float4 texColor = uiTexture.Sample(LinearSampler, input.uv);
    if (texColor.w < 0.5)
    {
        discard;
    }
    return float4(lerp(blendFactor, color.xyz, texColor.xyz), color.w);
}