
#define LIGHT_CONSTANTS\
    float4 mainLightDir;\
    float4 mainLightColor;\
    float4 shadowColor;\
    float3 ambientLight;\
    float ambientIntensity;\

// fogColor : color.xyz | density
// fogParams : fogStart | fogEnd
#define FOG_CONSTANTS\
    float4 fogColor;\
    float4 fogParams;