#include "common/Samplers.hlsl"

cbuffer PassConstants : register(b0)
{
    float4 time;
  
    float3 lightColor;
    float intensity;
	float4 lightDirection;
}

cbuffer ObjectConstants : register(b2)
{
	float4x4 m_model;
	float4x4 m_view;
	float4x4 m_proj;
}

struct VertexInput
{
    float3 position : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 biTangent : BITANGENT;
    float2 uv : TEXCOORD;
};

struct SimpleVertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

struct FragInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

FragInput VsMain(SimpleVertexInput input)
{
    FragInput o;
    float4 worldPosition = mul(m_model, float4(input.position, 1));
    o.position = mul(m_proj, mul(m_view, worldPosition));
    o.color = float4(input.position + 0.5, 1);
    //o.position = mul(float4(input.position, 1), objectTransform.m_model);
    //o.position = float4(input.position, 1);
    return o;
}

float4 PsMain(FragInput input) : SV_TARGET
{
    return input.color;
}
