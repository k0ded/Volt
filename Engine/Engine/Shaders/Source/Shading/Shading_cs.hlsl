#include "Defines.hlsli"
#include "Resources.hlsli"

#include "GPUScene.hlsli"
#include "Structures.hlsli"
#include "Utility.hlsli"
#include "Gradients.hlsli"

#include "PBR.hlsli"

#include "Volumetrics/Fog/VolumetricFogCommon.hlsli"

vt::RWTex2D<float4> RWOutput; 

vt::Tex2D<float4> Albedo;
vt::Tex2D<float3> Normals;
vt::Tex2D<float2> Material;
vt::Tex2D<float3> Emissive;
vt::Tex2D<uint> AOTexture;
vt::Tex2D<float> DepthTexture;

vt::UniformBuffer<VolumetricFogParams> VolumetricFogParamsData;
vt::Tex3D<float4> IntegratedFogVolume;
vt::TextureSampler PointSampler;

PBRConstants PBRConstantsData;

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
    const ViewData viewData = PBRConstantsData.viewData.Load();

    uint2 size = viewData.renderSize;
    
    if (any(threadId.xy >= size))
    {
        return;
    }
     
    const float4 albedo = Albedo.Load(int3(threadId.xy, 0));
    
    if (albedo.a < 0.5f)
    {
        return;
    }
    
    const float2 texCoords = float2(float(threadId.x) * viewData.invRenderSize.x, 1.f - float(threadId.y) * viewData.invRenderSize.y);
    
    const float2 material = Material.Load(int3(threadId.xy, 0));
    
    const float metallic = material.x;
    const float roughness = material.y;
    const float3 emissive = Emissive.Load(int3(threadId.xy, 0));
    const float3 normal = normalize(Normals.Load(int3(threadId.xy, 0)) * 2.f - 1.f);
    const float ao = CalculateAO(AOTexture, threadId.xy);    

    const float pixelDepth = DepthTexture.Load(int3(threadId.xy, 0));
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
     
    float3 outputColor = EvaluatePBR(pbrInput, PBRConstantsData);

    const VolumetricFogParams volumetricFogParams = VolumetricFogParamsData.Load();
    //outputColor = ApplyVolumetricFog(texCoords, pixelDepth, outputColor, viewData, volumetricFogParams, constants.pointSampler, constants.integratedFogVolume);

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

    RWOutput.Store(threadId.xy, float4(outputColor, 1.f));
}