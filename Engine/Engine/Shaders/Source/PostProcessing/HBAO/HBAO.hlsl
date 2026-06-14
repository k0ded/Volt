#include "ViewData.hlsli"
#include "HBAOConstants.hlsli"
#include "MathConstants.hlsli"

#define SAMPLES_PER_DIRECTION 4
#define DIRECTION_COUNT 8


#define USE_INLINE_PARAMETERS
#ifdef USE_INLINE_PARAMETERS
INLINE_PARAMETER_BLOCK({
    float2 jitter;
    float2 rotation;
    uint arrayIndex;
});
#else
float2 jitter;
float2 rotation;
uint arrayIndex;
#endif

uint GetArrayIndex()
{
#ifdef USE_INLINE_PARAMETERS
    return InlineParameters.arrayIndex;
#else
    return arrayIndex;
#endif
}

float2 GetJitter()
{
#ifdef USE_INLINE_PARAMETERS
    return InlineParameters.jitter;
#else
    return jitter;
#endif
}

float2 GetRotation()
{
#ifdef USE_INLINE_PARAMETERS
    return InlineParameters.rotation;
#else
    return rotation;
#endif
}

Texture2DArray<float> HBAODeinterleavedDepth;
Texture2D<float3> HBAOViewspaceNormals;
SamplerState PointClampSampler;

float3 ReconstructViewPosFromUV(float2 uv)
{
    //float depth = HBAODeinterleavedDepth.SampleLevel(PointClampSampler, float3(0, 0, 0), 0);
    //float depth = HBAODeinterleavedDepth[0.rrr];
    float depth = 1000;
    float2 ray = float2(uv * 2.0f - 1.0f) / float2(View.projection[0][0], View.projection[1][1]);
    return float3(ray * depth, depth);
}

half3 GetViewspaceNormal(float2 uv)
{
    return HBAOViewspaceNormals.SampleLevel(PointClampSampler, uv, 0) * 2 - 1;
}

// Inverse square attenuation
float AttenuationFunction(float viewSpaceDistance2)
{
    return max(0, viewSpaceDistance2 * constants.InvNegR2 + 1);
}

float2 RotateDirection(float2 D, float2 R)
{
    return float2(D.x * R.x + D.y * R.y, D.x * R.y + D.y * R.x);
}

float ComputeAO(float3 P, float3 N, float3 S)
{
    float3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * rsqrt(VdotV);

    // Use saturate(x) instead of max(x,0.f) because that is faster
    return saturate(NdotV - 0.5f) * saturate(AttenuationFunction(VdotV));
}

void AccumulateAO(
    inout float ao, 
    inout float RayPixels,
    float StepSizePixels,
    float2 Direction,
    float2 FullResUV,
    float3 ViewPosition,
    float3 ViewNormal
)
{
    float2 snappedUV = round(RayPixels * Direction) * constants.InvAOSize + FullResUV;

    float3 S = ReconstructViewPosFromUV(snappedUV);
    RayPixels += StepSizePixels;
    ao += ComputeAO(ViewPosition, ViewNormal, S);
}

#define USE_RANDOM_TEXTURE 0
#define USE_BLUE_NOISE 0
#define USE_CONSTANT 1

float ComputeHBAO(float2 fullResUV, float3 viewPosition, float3 viewNormal, float pixelRadius)
{
    // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    float StepSizePixels = pixelRadius / (SAMPLES_PER_DIRECTION + 1);

    const float alpha = 2.0 * PI / DIRECTION_COUNT;
    float SmallScaleAO = 0;
    float LargeScaleAO = 0;

    [unroll]
    for (float DirectionIndex = 0; DirectionIndex < DIRECTION_COUNT; ++DirectionIndex)
    {
        float Angle = alpha * DirectionIndex;

        // Compute normalized 2D direction
        float2 Direction = RotateDirection(float2(cos(Angle), sin(Angle)), GetRotation());

        // Jitter starting sample within the first step
        float RayPixels = (GetJitter().x * StepSizePixels + 1.0);

        {
            AccumulateAO(SmallScaleAO, RayPixels, StepSizePixels, Direction, fullResUV, viewPosition, viewNormal);
        }

        [unroll]
        for (float StepIndex = 1; StepIndex < SAMPLES_PER_DIRECTION; ++StepIndex)
        {
            AccumulateAO(LargeScaleAO, RayPixels, StepSizePixels, Direction, fullResUV, viewPosition, viewNormal);
        }
    }

    float AO = (SmallScaleAO * 2) + (LargeScaleAO * 2);

    AO /= (DIRECTION_COUNT * SAMPLES_PER_DIRECTION);
    return AO;
}

float2 MainPS(
	in float4 pos : SV_Position,
	in uint LayerIndex : SV_RenderTargetArrayIndex
) : SV_Target
{   
    pos.xy = floor(pos.xy) * 4.0 + float2(GetArrayIndex() % 4, GetArrayIndex() / 4);
    const float2 uv = pos.xy * constants.InvFullSize;
    const float3 viewPosition = ReconstructViewPosFromUV(uv);
    const float3 normal = GetViewspaceNormal(uv);

    // Assuming the monitor is a rectangle with the x axis being longer. 
    float radiusPixels = constants.radius * 0.5 * View.projection[1][1] / (viewPosition.z) * constants.AOSize.y; 

    [branch]
    if (radiusPixels < 1)
    {
        return 1.0f;
    }

    float AO = ComputeHBAO(uv, viewPosition, normal, radiusPixels);
    return float2(saturate(1 - AO), viewPosition.z);
}