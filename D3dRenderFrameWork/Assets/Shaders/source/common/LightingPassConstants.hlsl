struct LightPassConstants
{
    float4 mainLightDir;
    float4 mainLightColor;
    float4 ambientColor;
    float4 shadowColor;
    float mainLightIntensity;
    float3 ambientLight;              // 环境光颜色
    float ambientIntensity;           // 环境光强度

    float3 fogColor;
    float fogStart;
    float fogEnd;
    float fogDensity;
    uint fogMode;                     // 0: 无, 1: 线性, 2: 指数, 3: 指数平方
};