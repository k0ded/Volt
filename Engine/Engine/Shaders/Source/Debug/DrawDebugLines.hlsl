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

float4 MainPS(in VSToPS input) : SV_Target0
{
	return input.color;
}