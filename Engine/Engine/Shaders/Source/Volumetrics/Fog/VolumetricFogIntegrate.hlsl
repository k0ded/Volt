#include "Resources.hlsli"
#include "VolumetricFogCommon.hlsli"

vt::RWTex3D<float4> RWIntegratedVolume;
vt::Tex3D<float4> LightScattering;
vt::UniformBuffer<ViewData> View;
vt::UniformBuffer<VolumetricFogParams> VolumetricFogParamsData;
vt::TextureSampler PointSampler;

[numthreads(8, 8, 1)]
void MainCS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const VolumetricFogParams fogParams = VolumetricFogParamsData.Load();

    const float3 rcpFroxelDimensions = 1.f / float3(fogParams.froxelVolumeDimensions);

    int3 froxelCoord = dispatchThreadID;
    float3 integratedScattering = 0.f;
    float integratedTransmittance = 1.f;
    float currentZ = 0.f;

    for (int z = 0; z < fogParams.froxelVolumeDimensions.z; ++z)
    {
        froxelCoord.z = z;
        
        // Find distance between cells 
        float nextZ = SliceToExponentialDepth(fogParams.froxelNearPlane, fogParams.froxelFarPlane, z + 1.f, fogParams.froxelVolumeDimensions.z);
        const float zStep = abs(nextZ - currentZ);
        currentZ = nextZ;

        // Calculate scattering and transmittance for current cell
        const float4 scatteringExtinction = LightScattering.SampleLevel(PointSampler, float3(froxelCoord) * rcpFroxelDimensions, 0.f);
    
        const float transmittance = exp(-scatteringExtinction.w * zStep);
        const float3 scattering = (scatteringExtinction.rgb - scatteringExtinction.rgb * transmittance) / max(scatteringExtinction.w, 0.00001f);

        integratedScattering += scattering * integratedTransmittance;
        integratedTransmittance *= transmittance;

        RWIntegratedVolume.Store(froxelCoord, float4(integratedScattering, integratedTransmittance));
    }
}