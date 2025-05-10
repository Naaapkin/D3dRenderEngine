#define BEGIN_INSTANCE_DATA struct InstanceData {\
    float4x4 m_model;\
	float4x4 m_model_i;

#define END_INSTANCE_DATA };\
StructuredBuffer<InstanceData> InstanceBuffer : register(t0);

#define BEGIN_PASS_DATA cbuffer PassConstants : register(b0) {
#define END_PASS_DATA }

#define BEGIN_VIEW_DATA cbuffer CameraConstants : register(b1) {\
    float4x4 m_view;\
    float4x4 m_view_i;\
    float4x4 m_projection;\
    float4x4 m_projection_i;\
    float4   m_viewport;

#define END_VIEW_DATA };

#define BEGIN_OBJECT_DATA cbuffer ObjectConstants : register(b2) {\
    float4x4 m_model;\
    float4x4 m_model_i;\

#define END_OBJECT_DATA };

#define BEGIN_MATERIAL_DATA(name, x) cbuffer name : register(x) {
#define END_MATERIAL_DATA }