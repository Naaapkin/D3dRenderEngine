#include "common/Samplers.hlsl"

static const float2 positions[4] =
{
    float2(-1.0, 1.0), // ����
    float2(1.0, 1.0), // ����
    float2(-1.0, -1.0), // ����
    float2(1.0, -1.0) // ����
};

static const float2 uvs[4] =
{
    float2(0.0, 0.0), // ���� UV
    float2(1.0, 0.0), // ���� UV
    float2(0.0, 1.0), // ���� UV
    float2(1.0, 1.0) // ���� UV
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOutput VS(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.uv = uvs[vertexID];
    return output;
}

// ������������UI��ɫ��������
cbuffer UIConstants : register(b1)
{
    float4 m_color;
    float useTexture;
};

Texture2D uiTexture : register(t1);

float4 PS(VSOutput input) : SV_Target
{
    if (useTexture > 0.5)
    {
        return step(useTexture, 0.5) * uiTexture.Sample(LinearSampler, input.uv) * m_color;
    }
    return m_color; // ֱ�ӷ��ع̶���ɫ
}