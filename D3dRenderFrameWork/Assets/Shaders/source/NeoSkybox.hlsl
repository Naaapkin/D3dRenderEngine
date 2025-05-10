#include "common/Common.hlsl"
#include "common/LightingPassConstants.hlsl"

BEGIN_PASS_DATA
	LIGHT_CONSTANTS
END_PASS_DATA

BEGIN_VIEW_DATA
END_VIEW_DATA

BEGIN_MATERIAL_DATA(SkyConstants, b3)
    float SkyIntensity;
    float SunSize;
    float SunSizeConvergence;
    float AtmosphereThickness;
    float4 SkyTint;
    float4 GroundColor;
END_MATERIAL_DATA

static const float2 positions[4] =
{
    float2(-1.0, -1.0), // 左下
    float2(1.0, -1.0), // 右下
    float2(1.0, 1.0), // 右上
    float2(-1.0, 1.0) // 左上
};

float4 VsMain(uint vertexID : SV_VertexID) : SV_POSITION
{
    return float4(positions[vertexID], 0, 1.0);
}

float4 PsMain(float4 positionCS : SV_POSITION) : SV_Target
{
	// to NDC space
    positionCS.xy = (positionCS.xy / m_viewport.xy) * 2 - 1;
	// to homogenous scissor space
    positionCS.y = -positionCS.y;
    positionCS = mul(m_view_i, mul(m_projection_i, positionCS));
    float3 viewDir = normalize(positionCS);

    float skyDot = saturate(dot(viewDir, float3(0, 1, 0)));
    float sunIntensity = exp(-pow(length(viewDir - mainLightDir.xyz) / SunSize, SunSizeConvergence));

    // 渐变天空颜色（SkyTint 淡蓝色到白色）
    float3 skyColor = lerp(GroundColor.rgb, SkyTint.rgb, pow(skyDot, AtmosphereThickness));
    skyColor += sunIntensity * mainLightColor.rgb;

    return float4(skyColor, 1.0f);;
    //return float4(sunGlow.xxx, 1);
    //return float4(viewDir, 1);
}