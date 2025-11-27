#include "Utility/Utility.hlsli"

#include "GlobalIlluminationCommon.hlsli"

struct VSToPS
{
	float4 position : SV_Position;
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

	return output;
} 

struct PSOutput
{
    [[vt::rgba16f]] float4 sceneColor : SV_Target0;
    [[vt::d32f]];
};

PSOutput VisualizeIrradianceVolumePS(VSToPS input)
{
	PSOutput output;
	output.sceneColor = float4(1.f, 1.f, 1.f, 1.f);

	return output;
}