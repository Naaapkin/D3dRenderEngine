`// common/Transform.hlsl
cbuffer Transform : register(b0)
{
    float4x4 m_model;
    float4x4 m_i_model;
    float4x4 m_view;
    float4x4 m_i_view;
    float4x4 m_proj;
    float4x4 m_i_proj;
    float4x4 m_mvp;
    float4x4 m_i_mvp;
};