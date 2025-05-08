#include "common/Common.hlsl"
#include "common/LightingPassConstants.hlsl"

BEGIN_PASS_DATA
	LIGHT_CONSTANTS
END_PASS_DATA

cbuffer CameraBuffer : register(b1)
{
    float4x4 ViewProj;
    float4x4 ViewProj_I;
}

BEGIN_MATERIAL_DATA(b2)
    float SkyIntensity;
    float SunSize;
    float SunSizeConvergence;
    float AtmosphereThickness;
    float4 SkyTint;
    float4 GroundColor;

    float4 SunColor;
    float3 SunDirection;
END_MATERIAL_DATA

static const float2 positions[4] =
{
    float2(-1.0, 1.0), // 左上
    float2(1.0, 1.0), // 右上
    float2(-1.0, -1.0), // 左下
    float2(1.0, -1.0) // 右下
};

float4 VsMain(uint vertexID : SV_VertexID)
    {

    return float4(positions[vertexID], 0, 1);
}

// RGB wavelengths
    // .35 (.62=158), .43 (.68=174), .525 (.75=190)
static const float3 kDefaultScatteringWavelength = float3(.65, .57, .475);
static const float3 kVariableRangeForScatteringWavelength = float3(.15, .15, .15);

#define OUTER_RADIUS 1.025
static const float kOuterRadius = OUTER_RADIUS;
static const float kOuterRadius2 = OUTER_RADIUS * OUTER_RADIUS;
static const float kInnerRadius = 1.0;
static const float kInnerRadius2 = 1.0;

static const float kCameraHeight = 0.0001;

#define kRAYLEIGH (lerp(0.0, 0.0025, pow(AtmosphereThickness,2.5)))      // Rayleigh constant
#define kMIE 0.0010             // Mie constant
#define kSUN_BRIGHTNESS 20.0    // Sun brightness

#define kMAX_SCATTER 50.0 // Maximum scattering value, to prevent math overflows on Adrenos

static const float kHDSundiskIntensityFactor = 15.0;
static const float kSimpleSundiskIntensityFactor = 27.0;

static const float kSunScale = 400.0 * kSUN_BRIGHTNESS;
static const float kKmESun = kMIE * kSUN_BRIGHTNESS;
static const float kKm4PI = kMIE * 4.0 * 3.14159265;
static const float kScale = 1.0 / (OUTER_RADIUS - 1.0);
static const float kScaleDepth = 0.25;
static const float kScaleOverScaleDepth = (1.0 / (OUTER_RADIUS - 1.0)) / 0.25;
static const float kSamples = 2.0; // THIS IS UNROLLED MANUALLY, DON'T TOUCH

#define MIE_G (-0.990)
#define MIE_G2 0.9801

#define SKY_GROUND_THRESHOLD 0.02

    // Calculates the Rayleigh phase function
float getRayleighPhase(float eyeCos2)
{
    return 0.75 + 0.75 * eyeCos2;
}
float getRayleighPhase(float3 light, float3 ray)
{
    float eyeCos = dot(light, ray);
    return getRayleighPhase(eyeCos * eyeCos);
}

float scale(float inCos)
{
    float x = 1.0 - inCos;
    return 0.25 * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

    // Calculates the Mie phase function
float getMiePhase(float eyeCos, float eyeCos2)
{
    float temp = 1.0 + MIE_G2 - 2.0 * MIE_G * eyeCos;
    temp = pow(temp, pow(SunSize, 0.65) * 10);
    temp = max(temp, 1.0e-4); // prevent division by zero, esp. in float precision
    temp = 1.5 * ((1.0 - MIE_G2) / (2.0 + MIE_G2)) * (1.0 + eyeCos2) / temp;
    return temp;
}

    // Calculates the sun shape
float calcSunAttenuation(float3 lightPos, float3 ray)
{
    float focusedEyeCos = pow(saturate(dot(lightPos, ray)), SunSizeConvergence);
    return getMiePhase(-focusedEyeCos, focusedEyeCos * focusedEyeCos);
}

float4 RenderSky(float4 positionCS)
{
    float3 viewDirWS = mul(ViewProj_I, positionCS);

    // Reverse it to point into the scene
    float3 dir = -viewDirWS;

    float3 kScatteringWavelength = lerp(
            kDefaultScatteringWavelength - kVariableRangeForScatteringWavelength,
            kDefaultScatteringWavelength + kVariableRangeForScatteringWavelength,
            float3(1, 1, 1) - SkyTint.xyz); // using Tint in sRGB gamma allows for more visually linear interpolation and to keep (.5) at (128, gray in sRGB) point
    float3 kInvWavelength = 1.0 / float3(pow(kScatteringWavelength.x, 4), pow(kScatteringWavelength.y, 4), pow(kScatteringWavelength.z, 4));

    float kKrESun = kRAYLEIGH * kSUN_BRIGHTNESS;
    float kKr4PI = kRAYLEIGH * 4.0 * 3.14159265;

    float3 cameraPos = float3(0, kInnerRadius + kCameraHeight, 0); // The camera's current position

    // Get the ray from the camera to the vertex and its length (which is the far point of the ray passing through the atmosphere)
    float3 eyeRay = dir; // normalize(mul((float3x3)GetObjectToWorldMatrix(), v.vertex.xyz));

    float far = 0.0;
    float3 cIn = float3(0.0, 0.0, 0.0);
    float3 cOut = float3(0.0, 0.0, 0.0);

    float3 groundColor = float3(0.0, 0.0, 0.0);
    float3 skyColor = float3(0.0, 0.0, 0.0);

    // Modification for per-pixel procedural sky:
    // Contrary to the legacy version that is run per-vertex, this version is per pixel.
    // The fact that it was run per-vertex means that the colors were never computed at the horizon.
    // Now that it's per vertex, we reach the limitation of the computation at the horizon where a very bright line appears.
    // To avoid that, we clampe the height of the eye ray just above and below the horizon for sky and ground respectively.
    // Another modification to make this work was to add ground and sky contribution instead of lerping between them.
    // For this to work we also needed to change slightly the computation so that cIn and cOut factor computed for the sky did not affect ground and vice versa (it was the case before) so that we can add both contribution without adding energy
    float horizonThreshold = 0.02;
    if (eyeRay.y >= 0.0)
    {
        float3 clampedEyeRay = eyeRay;
        clampedEyeRay.y = max(clampedEyeRay.y, horizonThreshold);
            // Sky
            // Calculate the length of the "atmosphere"
        far = sqrt(kOuterRadius2 + kInnerRadius2 * clampedEyeRay.y * clampedEyeRay.y - kInnerRadius2) - kInnerRadius * clampedEyeRay.y;

        float3 pos = cameraPos + far * clampedEyeRay;

        // Calculate the ray's starting position, then calculate its scattering offset
        float height = kInnerRadius + kCameraHeight;
        float depth = exp(kScaleOverScaleDepth * (-kCameraHeight));
        float startAngle = dot(clampedEyeRay, cameraPos) / height;
        float startOffset = depth * scale(startAngle);

        // Initialize the scattering loop variables
        float sampleLength = far / kSamples;
        float scaledLength = sampleLength * kScale;
        float3 sampleRay = clampedEyeRay * sampleLength;
        float3 samplePoint = cameraPos + sampleRay * 0.5;

        // Now loop through the sample rays
        float3 frontColor = float3(0.0, 0.0, 0.0);
        for (int i = 0; i < int(kSamples); i++)
        {
            float sampleHeight = length(samplePoint);
            float sampleDepth = exp(kScaleOverScaleDepth * (kInnerRadius - sampleHeight));
            float lightAngle = dot(SunDirection.xyz, samplePoint) / sampleHeight;
            float cameraAngle = dot(clampedEyeRay, samplePoint) / sampleHeight;
            float scatter = (startOffset + sampleDepth * (scale(lightAngle) - scale(cameraAngle)));
            float3 attenuate = exp(-clamp(scatter, 0.0, kMAX_SCATTER) * (kInvWavelength * kKr4PI + kKm4PI));

            frontColor += attenuate * (sampleDepth * scaledLength);
            samplePoint += sampleRay;
        }

        // Finally, scale the Mie and Rayleigh colors and set up the varying variables for the pixel shader
        cIn = frontColor * (kInvWavelength * kKrESun);
        cOut = frontColor * kKmESun;

        skyColor = (cIn * getRayleighPhase(SunDirection.xyz, -eyeRay));
    }
    else
    {
        float3 clampedEyeRay = eyeRay;
        clampedEyeRay.y = min(clampedEyeRay.y, -horizonThreshold);
            // Ground
        far = (-kCameraHeight) / (min(-0.001, clampedEyeRay.y));

        float3 pos = cameraPos + far * clampedEyeRay;

        // Calculate the ray's starting position, then calculate its scattering offset
        float depth = exp((-kCameraHeight) * (1.0 / kScaleDepth));
        float cameraAngle = dot(-clampedEyeRay, pos);
        float lightAngle = dot(SunDirection.xyz, pos);
        float cameraScale = scale(cameraAngle);
        float lightScale = scale(lightAngle);
        float cameraOffset = depth * cameraScale;
        float temp = (lightScale + cameraScale);

        // Initialize the scattering loop variables
        float sampleLength = far / kSamples;
        float scaledLength = sampleLength * kScale;
        float3 sampleRay = clampedEyeRay * sampleLength;
        float3 samplePoint = cameraPos + sampleRay * 0.5;

        // Now loop through the sample rays
        float3 frontColor = float3(0.0, 0.0, 0.0);
        float3 attenuate;
        {
            float sampleHeight = length(samplePoint);
            float sampleDepth = exp(kScaleOverScaleDepth * (kInnerRadius - sampleHeight));
            float scatter = sampleDepth * temp - cameraOffset;
            attenuate = exp(-clamp(scatter, 0.0, kMAX_SCATTER) * (kInvWavelength * kKr4PI + kKm4PI));
            frontColor += attenuate * (sampleDepth * scaledLength);
            samplePoint += sampleRay;
        }

        cIn = frontColor * (kInvWavelength * kKrESun + kKmESun);
        cOut = clamp(attenuate, 0.0, 1.0);

        groundColor = (cIn + GroundColor.rgb * GroundColor.rgb * cOut);
    }

    float3 sunColor = float3(0.0, 0.0, 0.0);
    float3 col = float3(0.0, 0.0, 0.0);
    float y = -eyeRay.y / SKY_GROUND_THRESHOLD;

    col = groundColor + skyColor;
    return float4(col * SkyIntensity, 1.0);
}

float4 FragBaking(float4 positionCS : SV_POSITION) : SV_Target
{
    return RenderSky(positionCS);
}

float4 FragRender(float4 positionCS : SV_POSITION) : SV_Target
{
    return RenderSky(positionCS);
}