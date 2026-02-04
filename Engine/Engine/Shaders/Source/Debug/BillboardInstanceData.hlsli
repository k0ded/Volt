#pragma once

struct BillboardInstanceData
{
	float3 position;
	uint padding0;
	float3 size;
	uint isViewSpacePosition;
	float4 color;
	float4 userData;
};

struct BillboardVSToPS
{
	float4 position : SV_Position;
	float4 color : COLOR;
    float2 texCoords : TEXCOORD;
	nointerpolation float4 userData : USERDATA;
};