#include "common/Transform.hlsl"

Transform(b1)

struct SimpleVertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct FragInput
{
    float4 position : SV_POSITION;
};

FragInput VsMain(SimpleVertexInput input)
{
    FragInput o;
    float4 worldPosition = mul(m_model, float4(input.position, 1));
    o.position = mul(m_projection, mul(m_view, worldPosition));
    return o;
}

