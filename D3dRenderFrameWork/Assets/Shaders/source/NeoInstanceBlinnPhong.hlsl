#include "common/Samplers.hlsl"
#include "common/LightingPassConstants.hlsl"
#include "common/Common.hlsl"

BEGIN_PASS_DATA
	LIGHT_CONSTANTS
END_PASS_DATA

BEGIN_VIEW_DATA
END_VIEW_DATA

BEGIN_INSTANCE_DATA
END_INSTANCE_DATA

BEGIN_MATERIAL_DATA(DiffuseInfo, b3)
float4 m_diffuseBias;
END_MATERIAL_DATA

struct SimpleVertexInput
{
    float3 positionOS : POSITION;
    float3 normalOS : NORMAL;
    float2 uv : TEXCOORD;
    uint instanceId : SV_InstanceID;
};

struct FragInput
{
    float4 positionHCS : SV_POSITION;
    float3 positionVS : TEXCOORD1;
    float3 normalWS : NORMAL; // 世界法线
    float2 uv : POSITION;
};

Texture2D<float4> diffuse : register(t0);

FragInput VsMain(SimpleVertexInput input)
{
    FragInput o;
    float4 positionVS = mul(m_view, mul(InstanceBuffer[input.instanceId].m_model, float4(input.positionOS, 1)));
    o.positionVS = positionVS;
    o.positionHCS = mul(m_projection, positionVS);
    o.normalWS = mul((float3x3) m_view_i, input.normalOS);
    o.uv = input.uv * m_diffuseBias.xy + m_diffuseBias.zw;
    return o;
}

float4 PsMain(FragInput input) : SV_TARGET
{
    // 归一化法线和视图方向
    float3 normalW = normalize(input.normalWS);
    float3 viewDirW = normalize(input.positionVS);
    
    // 光照方向（假设是点光源）
    float3 lightDirW = normalize(-mainLightDir.xyz);
    
    // 环境光分量
    float3 ambient = ambientIntensity * ambientLight;
    
    // 漫反射分量（Lambert）
    float NdotL = max(dot(normalW, lightDirW), 0.0f);
    float3 diffuse = NdotL * mainLightColor.rgb * mainLightColor.w;
    
    // 高光分量（Blinn-Phong半角向量优化）
    float3 halfDir = normalize(lightDirW + viewDirW);
    float NdotH = max(dot(normalW, halfDir), 0.0f);
    float specPower = 32.0f; // 高光指数（控制锐利度）
    float3 specular = pow(NdotH, specPower) * diffuse;
    
    // 组合光照结果
    float3 litColor = (ambient.rgb + diffuse + specular);
    
    // 返回最终颜色（此处假设材质为白色，可结合纹理采样）
    return float4(litColor, 1.0f);
    //return diffuse.Sample(LinearSampler, input.uv);
}

