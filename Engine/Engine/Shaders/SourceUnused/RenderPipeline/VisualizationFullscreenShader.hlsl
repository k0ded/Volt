#include "Defines.hlsli"
#include "Resources.hlsli"

#include "GPUScene.hlsli"
#include "Structures.hlsli"
#include "Utility.hlsli"
#include "Gradients.hlsli"

#include "PBR.hlsli"

#include "VisualizationCommon.hlsli"

vt::RWTex2D<float4> RWOutput;

vt::Tex2D<float4> Albedo;
vt::Tex2D<float2> Material;
vt::Tex2D<float3> SceneColor;
vt::Tex2D<float> SceneDepth;
vt::Tex2D<float3> SceneNormal;
vt::Tex2D<uint> SceneAO;
vt::Tex2D<float2> Velocity;

uint2 RenderSize;
uint VisualizationModeInt;

float CalculateAO(vt::Tex2D<uint> aoTex, uint2 pixelCoord)
{
#define XE_GTAO_OCCLUSION_TERM_SCALE (1.5f)      // for packing in UNORM (because raw, pre-denoised occlusion term can overshoot 1 but will later average out to 1)

    const float ao = (aoTex.Load(int3(pixelCoord, 0)).x >> 24) / 255.f;
    float finalAO = min(ao * XE_GTAO_OCCLUSION_TERM_SCALE, 1.f);

    return finalAO;
}

[numthreads(8, 8, 1)]
void MainCS(uint3 threadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex)
{
    if (any(threadId.xy >= RenderSize))
    {
        return;
    }

    float3 finalColor = 0.f;

    if (VisualizationModeInt == EVisualizationMode::ERM_BaseColor)
    {
        finalColor = Albedo.Load(int3(threadId.xy, 0)).rgb;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_Metallic)
    {
        float2 material = Material.Load(int3(threadId.xy, 0));
        finalColor = material.xxx;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_Roughness)
    {
        float2 material = Material.Load(int3(threadId.xy, 0));
        finalColor = material.yyy;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_SceneColor)
    {
        finalColor = SceneColor.Load(int3(threadId.xy, 0));
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_SceneDepth)
    {
        float depth = SceneDepth.Load(int3(threadId.xy, 0));
        finalColor = depth.xxx;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_WorldNormal)
    {
        finalColor = SceneNormal.Load(int3(threadId.xy, 0));
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_AmbientOcclusion)
    {
        const float ao = CalculateAO(SceneAO, threadId.xy);    
        finalColor = ao.xxx;
    }
    else if (VisualizationModeInt == EVisualizationMode::ERM_Velocity)
    {
        const float2 velocity = Velocity.Load(int3(threadId.xy, 0));
        finalColor.rg = velocity;
    }

    RWOutput.Store(threadId.xy, float4(finalColor, 1.f));
}