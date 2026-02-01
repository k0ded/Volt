#pragma once

struct BillboardInstanceData
{
	float3 position;
	uint userData;
	float3 size;
	uint isViewSpacePosition;
	float4 color;
};

struct BillboardVSToPS
{
	float4 position : SV_Position;
	float4 color : COLOR;
    float2 texCoords : TEXCOORD;
	uint userData : USERDATA;
};