// common/LightConstants.hlsl
cbuffer LightConstants : register(b1)
{
    float4 lightDir;
    float4 lightColor;
    float4 ambientColor;
    float4 shadowColor;
    float lightIntensity;
};