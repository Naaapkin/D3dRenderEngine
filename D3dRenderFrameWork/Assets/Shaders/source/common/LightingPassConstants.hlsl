
#define LIGHT_CONSTANTS(x)\
    float4 mainLightDir;\
    float4 mainLightColor;\
    float4 ambientColor;\
    float4 shadowColor;\
    float mainLightIntensity;\
    float3 ambientLight;\
    float ambientIntensity;\
\
    float3 fogColor;\
    float fogStart;\
    float fogEnd;\
    float fogDensity;\
    uint fogMode;