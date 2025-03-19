#include "assets/Shaders/PC/common/Transform.hlsl"

struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD;
    float4 texcoord : TEXCOORD1;
};

struct FragInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

FragInput main(VertexInput input) : SV_TARGET
{
    FragInput o;
    float4 worldPosition = mul(m_model, float4(input.position, 1));
    o.position = mul(m_proj, mul(m_view, worldPosition));
    o.color = float4(input.position + 0.5, 1);
    //o.position = mul(float4(input.position, 1), objectTransform.m_model);
    //o.position = float4(input.position, 1);
    return o;
}
