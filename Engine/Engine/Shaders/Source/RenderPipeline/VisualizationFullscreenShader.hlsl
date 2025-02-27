#include "Defines.hlsli"
#include "Resources.hlsli"

#include "GPUScene.hlsli"
#include "Structures.hlsli"
#include "Utility.hlsli"
#include "Gradients.hlsli"

#include "PBR.hlsli"

#include "VisualizationCommon.hlsli"

struct Constants
{
    vt::RWTex2D<float4> rwOutput;
    
    vt::Tex2D<float4> albedo;
    vt::Tex2D<float2> material;
    vt::Tex2D<float3> sceneColor;
    vt::Tex2D<float> sceneDepth;
    vt::Tex2D<float3> sceneNormal;
    vt::Tex2D<uint> sceneAO;
    vt::Tex2D<float2> velocity;
    
    uint2 renderSize;
    uint visualizationMode;
};

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
    const Constants constants = GetConstants<Constants>();
    
    uint2 size = constants.renderSize;
    
    if (any(threadId.xy >= size))
    {
        return;
    }

    float3 finalColor = 0.f;

    if (constants.visualizationMode == VisualizationMode::BaseColor)
    {
        finalColor = constants.albedo.Load(int3(threadId.xy, 0)).rgb;
    }
    else if (constants.visualizationMode == VisualizationMode::Metallic)
    {
        float2 material = constants.material.Load(int3(threadId.xy, 0));
        finalColor = material.xxx;
    }
    else if (constants.visualizationMode == VisualizationMode::Roughness)
    {
        float2 material = constants.material.Load(int3(threadId.xy, 0));
        finalColor = material.yyy;
    }
    else if (constants.visualizationMode == VisualizationMode::SceneColor)
    {
        finalColor = constants.sceneColor.Load(int3(threadId.xy, 0));
    }
    else if (constants.visualizationMode == VisualizationMode::SceneDepth)
    {
        float depth = constants.sceneDepth.Load(int3(threadId.xy, 0));
        finalColor = depth.xxx;
    }
    else if (constants.visualizationMode == VisualizationMode::WorldNormal)
    {
        finalColor = constants.sceneNormal.Load(int3(threadId.xy, 0));
    }
    else if (constants.visualizationMode == VisualizationMode::AmbientOcclusion)
    {
        const float ao = CalculateAO(constants.sceneAO, threadId.xy);    
        finalColor = ao.xxx;
    }
    else if (constants.visualizationMode == VisualizationMode::Velocity)
    {
        const float2 velocity = constants.velocity.Load(int3(threadId.xy, 0));
        finalColor.rg = velocity;
    }

    constants.rwOutput.Store(threadId.xy, float4(finalColor, 1.f));
}