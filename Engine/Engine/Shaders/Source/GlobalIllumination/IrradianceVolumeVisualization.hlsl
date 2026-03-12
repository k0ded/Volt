#include "Utility/Utility.hlsli"

#include "GlobalIlluminationCommon.hlsli"

#include "MonteCarlo.hlsli"

struct VSToPS
{
	float4 position : SV_Position;
	float3 localPosition : POSITION;
	uint probeId : PROBEID;
};

struct VSInput
{
    [[vt::inputIndex(0)]] float3 position : POSITION;
};

uint IrradianceVolumeCascadeIndex;

VSToPS VisualizeIrradianceVolumeVS(VSInput input, uint instanceId : SV_InstanceID)
{
	const uint3 probeCoords = IrradianceVolume::GetProbeCoordsFromProbeIndex(instanceId);

	const float3 probePosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeCoords, IrradianceVolumeCascadeIndex);
	const float3 vertexPosition = probePosition + input.position * IrradianceVolume::GetCascadeProbeSpacing(IrradianceVolumeCascadeIndex) * 0.05f;

	VSToPS output;
	output.position = mul(View.viewProjection, float4(vertexPosition, 1.f));
	output.localPosition = input.position;
	output.probeId = instanceId;
	
	return output;
} 

Texture2D<float3> ProbeAtlas;

float4 VisualizeIrradianceVolumePS(VSToPS input) : SV_Target0
{
	const float3 direction = normalize(input.localPosition);
	const float2 uv = InverseEquiAreaSphericalMapping(direction);

	const uint2 probeAtlasCoords = IrradianceVolume::GetProbeAtlasTexelCoordsFromProbeIndex(input.probeId, IrradianceVolumeCascadeIndex);

	const uint2 localTexelCoords = uv * float(IrradianceVolumeProbeResolution);
	const float3 texelRadiance = ProbeAtlas[probeAtlasCoords + localTexelCoords + 1];

	return float4(texelRadiance, 1.f);
}