#include "ViewData.hlsli"

struct LineVertex
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VSToPS
{
	float4 position : SV_Position;
	float4 color : COLOR;
};

VSToPS MainVS(in LineVertex input)
{
	VSToPS result;
	result.position = mul(View.viewProjection, float4(input.position, 1.f));
	result.color = input.color;

	return result;
}

struct PSOutput
{
    [[vt::rgba8]] float4 color : SV_Target0;
    [[vt::d32f]];
};

PSOutput MainPS(in VSToPS input)
{
	PSOutput output;
	output.color = input.color;

	return output;
}