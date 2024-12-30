#include "Defines.hlsli"
#include "Resources.hlsli"

#include "GPUScene.hlsli"
#include "Structures.hlsli"
#include "Utility.hlsli"
#include "Gradients.hlsli"

#include "PBR.hlsli"

struct Constants
{
    vt::RWTex2D<float4> output;
    
    vt::Tex2D<float4> albedo;
    vt::Tex2D<float3> normals;
    vt::Tex2D<float2> material;
    vt::Tex2D<float3> emissive;
    vt::Tex2D<uint> aoTexture;
    vt::Tex2D<float> depthTexture;

    PBRConstants pbrConstants;
};

float CalculateAO(vt::Tex2D<uint> aoTex, uint2 pixelCoord)
{
#define XE_GTAO_OCCLUSION_TERM_SCALE (1.5f)      // for packing in UNORM (because raw, pre-denoised occlusion term can overshoot 1 but will later average out to 1)

    const float ao = (aoTex.Load(int3(pixelCoord, 0)).x >> 24) / 255.f;
    float finalAO = min(ao * XE_GTAO_OCCLUSION_TERM_SCALE, 1.f);

    return finalAO;
}

[numthreads(8, 8, 1)]
void main(uint3 threadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex)
{
    const Constants constants = GetConstants<Constants>();
    const ViewData viewData = constants.pbrConstants.viewData.Load();

    uint2 size = viewData.renderSize;
    
    if (any(threadId.xy >= size))
    {
        return;
    }
    
    const float4 albedo = constants.albedo.Load(int3(threadId.xy, 0));
    
    if (albedo.a < 0.5f)
    {
        return;
    }
    
    const float2 texCoords = float2(float(threadId.x) * viewData.invRenderSize.x, 1.f - float(threadId.y) * viewData.invRenderSize.y);
    
    const float2 material = constants.material.Load(int3(threadId.xy, 0));
    
    const float metallic = material.x;
    const float roughness = material.y;
    const float3 emissive = constants.emissive.Load(int3(threadId.xy, 0));
    const float3 normal = normalize(constants.normals.Load(int3(threadId.xy, 0)) * 2.f - 1.f);
    const float ao = CalculateAO(constants.aoTexture, threadId.xy);    

    const float pixelDepth = constants.depthTexture.Load(int3(threadId.xy, 0));
    const float3 worldPosition = ReconstructWorldPosition(viewData, texCoords, pixelDepth);
    
    PBRInput pbrInput;
    pbrInput.albedo = albedo;
    pbrInput.normal = normal;
    pbrInput.metallic = metallic;
    pbrInput.roughness = roughness;
    pbrInput.emissive = emissive;
    pbrInput.worldPosition = worldPosition;
    pbrInput.ao = ao;
    pbrInput.tileId = threadId.xy / LIGHT_CULLING_TILE_SIZE;
    
    float3 outputColor = EvaluatePBR(pbrInput, constants.pbrConstants);

    //switch (constants.visualizationMode)
    //{
    //    case VisualizationMode::VisualizeCascades:
    //    {
    //        outputColor = outputColor * 0.2f + GetCascadeColorFromIndex(GetCascadeIndexFromWorldPosition(constants.pbrConstants.directionalLight.Load(), worldPosition, viewData.view)) * 0.5f;
    //        break;
    //    }
    //
    //    case VisualizationMode::VisualizeLightComplexity:
    //    {
    //        outputColor = outputColor * 0.2f + GetLightComplexityGradient(GetLightCount(constants.pbrConstants.visiblePointLights, viewData.tileCountX, pbrInput.tileId));
    //        break;
    //    }
    //
    //    default:
    //        break;
    //};

    constants.output.Store(threadId.xy, float4(outputColor, 1.f));
}