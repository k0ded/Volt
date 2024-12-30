#include "Resources.hlsli"
#include "Structures.hlsli"

struct Constants
{
    vt::Tex2D<float3> sceneNormals;
    vt::Tex2D<float> sceneDepth;
    vt::Tex2D<float2> sceneMaterial;

    vt::TextureSampler pointSampler;
    vt::UniformBuffer<ViewData> viewData;

    vt::RWTex2D<float3> rwOutput;
};

float3 ReconstructViewPosition(float2 texCoord, float depth, in float4x4 invProjMat)
{
    float4 clipSpace = float4(texCoord * 2.f - 1.f, depth, 1.f);
    float4 viewSpace = mul(invProjMat, clipSpace);
    return viewSpace.xyz / viewSpace.w;
}

float3 ComputeReflectionDirection(float3 viewPos, float3 normal)
{
    float3 viewDir = normalize(-viewPos);
    return reflect(viewDir, normal);
}

float2 ScreenSpaceRayMarch(float3 viewPos, float3 rayDir, vt::Tex2D<float> sceneDepth, vt::TextureSampler pointSampler, float4x4 projMat, float4x4 invProjMat)
{
    const int MaxSteps = 100;
    const float StepSize = 10.f;

    float3 currentPos = viewPos;
    float2 texCoord;

    for (int i = 0; i < MaxSteps; ++i)
    {
        float4 projectedPos = mul(projMat, float4(currentPos, 1.f));
        texCoord = (projectedPos.xy / projectedPos.w) * 0.5f + 0.5f;

        texCoord.y = 1.f - texCoord.y;

        if (any(texCoord < 0.f) || any(texCoord > 1.f))
        {
            break;
        }

        float pixelDepth = sceneDepth.SampleLevel(pointSampler, texCoord, 0.f);
        float3 pixelPos = ReconstructViewPosition(texCoord, pixelDepth, invProjMat);
        
        if (currentPos.z <= pixelPos.z)
        {
            return texCoord;
        }

        currentPos += rayDir * StepSize;
    }

    return -1.f;
}

[numthreads(8, 8, 1)]
void SSRCS(uint2 dispatchThreadId : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
    const ViewData viewData = constants.viewData.Load();    

    float2 texCoords = (float2(dispatchThreadId) + 0.5f) * viewData.invRenderSize;
    texCoords.y = 1.f - texCoords.y;

    float3 pixelNormal = constants.sceneNormals.SampleLevel(constants.pointSampler, texCoords, 0.f) * 2.f - 1.f;
    pixelNormal = mul(float4(pixelNormal, 0.f), viewData.inverseView).xyz;

    float pixelDepth = constants.sceneDepth.SampleLevel(constants.pointSampler, texCoords, 0.f);
    float pixelRoughness = constants.sceneMaterial.SampleLevel(constants.pointSampler, texCoords, 0.f).g;

    if (pixelDepth > 0.f && pixelRoughness < 0.5f)
    {
        float3 viewPos = ReconstructViewPosition(texCoords, pixelDepth, viewData.inverseProjection);
        float3 reflectionDir = ComputeReflectionDirection(viewPos, pixelNormal);
        
        float2 reflectionTexCoord = ScreenSpaceRayMarch(viewPos, reflectionDir, constants.sceneDepth, constants.pointSampler, viewData.projection, viewData.inverseProjection);

        if (all(reflectionTexCoord >= 0.f) && all(reflectionTexCoord.y >= 0.f))
        {
            reflectionTexCoord.y = 1.f - reflectionTexCoord.y;

            float3 reflectedColor = constants.rwOutput.Load(reflectionTexCoord * viewData.renderSize);
            constants.rwOutput.Store(texCoords * viewData.renderSize, reflectedColor); 
        }
    }
}