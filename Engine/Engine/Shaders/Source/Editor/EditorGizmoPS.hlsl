#include "ViewData.hlsli"
#include "Debug/BillboardInstanceData.hlsli"
#include "StaticSamplerStates.hlsli"

struct PSOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
	[[vt::r32ui]] uint objectId : SV_Target1;
    [[vt::d32f]];
};

Texture2D Texture;

PSOutput MainPS(in BillboardVSToPS input)
{
    const float4 textureColor = Texture.Sample(StaticAnisotropicSamplerClamp, input.texCoords);

	PSOutput output;
	output.color = textureColor * input.color;
	output.objectId = input.userData;

	return output;
}