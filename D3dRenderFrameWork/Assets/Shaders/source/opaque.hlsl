#include "common/Samplers.hlsl"
#include "common/Common.hlsl"

BEGIN_OBJECT_DATA
END_OBJECT_DATA

BEGIN_MATERIAL_DATA(b2)
    float4 m_diffuseBias;
END_MATERIAL_DATA

struct SimpleVertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct FragInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

Texture2D<float4> diffuse : register(t0);

FragInput VsMain(SimpleVertexInput input)
{
    FragInput o;
    float4 worldPosition = mul(m_model, float4(input.position, 1));
    o.position = mul(m_projection, mul(m_view, worldPosition));
    o.uv = input.uv * m_diffuseBias.xy + m_diffuseBias.zw;
    return o;
}

float4 PsMain(FragInput input) : SV_TARGET
{
    //return input.position;
    return diffuse.Sample(LinearSampler, input.uv);
}

