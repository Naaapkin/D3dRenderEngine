struct PBRMaterial
{
    // 基础颜色和透明度
    float4 baseColorFactor;       // RGB: 基础颜色, A: 透明度
    
    // 金属度/粗糙度工作流参数
    float metallicFactor;         // 金属度系数 [0, 1]
    float roughnessFactor;        // 粗糙度系数 [0, 1]
    float occlusionStrength;      // 环境光遮蔽强度系数 [0, 1]
    float normalScale;            // 法线贴图缩放系数
    
    // 自发光
    float3 emissiveFactor;        // 自发光颜色
    float emissiveIntensity;      // 自发光强度
    
    // 附加控制参数
    float ior;                    // 折射率 (通常为1.5)
    float clearcoatFactor;        // 清漆系数 [0, 1]
    float clearcoatRoughness;     // 清漆粗糙度 [0, 1]
    float subsurfaceScattering;   // 次表面散射强度 [0, 1]
    
    // Alpha裁剪/混合控制
    float alphaCutoff;            // Alpha裁剪阈值
    uint alphaMode;               // 0: 不透明, 1: 裁剪, 2: 混合
    
    // 纹理使用标志 (位掩码)
    uint textureFlags;            // 位0: 基础颜色, 位1: 金属度/粗糙度, 位2: 法线...
    
    // 纹理坐标变换
    float4 baseColorTexCoordTransform;  // XY: 缩放, ZW: 偏移
    float4 metallicRoughnessTexCoordTransform;
    float4 normalTexCoordTransform;
    float4 emissiveTexCoordTransform;
    float4 occlusionTexCoordTransform;
    
    // 保留用于将来扩展或对齐
    float4 reserved[2];
};