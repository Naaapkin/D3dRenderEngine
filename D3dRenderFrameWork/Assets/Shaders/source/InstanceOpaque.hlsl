#include "common/Samplers.hlsl"
#include "common/Common.hlsl"

BEGIN_OBJECT_DATA
END_OBJECT_DATA

BEGIN_INSTANCE_DATA
    float4 m_diffuseBias;
END_INSTANCE_DATA

struct InstanceVertexInput
{
    float3 positionOS : POSITION;
    float3 normalOS : NORMAL;
    float2 uv : TEXCOORD;
    uint   instanceId : SV_InstanceID;
};

struct FragInput
{
    float4 positionHS : SV_POSITION;
    float3 normalWS : NORMAL;
    float2 uv : TEXCOORD0;
};

Texture2D<float4> diffuse : register(t1);

FragInput VsMain(InstanceVertexInput input)
{
    FragInput o;
    float4 worldPosition = mul(m_model, float4(input.positionOS, 1));
    o.positionHS = mul(m_projection, mul(m_view, worldPosition));
    o.normalWS = mul((float3x3) transpose(m_model_i), input.normalOS);
    float4 diffuseBias = InstanceBuffer[input.instanceId].m_diffuseBias;
    o.uv = input.uv * diffuseBias.xy + diffuseBias.zw;
    return o;
}

float4 PsMain(FragInput input) : SV_TARGET
{
    return diffuse.Sample(LinearSampler, input.uv);
}

