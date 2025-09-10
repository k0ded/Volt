#pragma once

struct Barycentrics
{
	void Initialize(float2 inBarycentrics)
	{
		barycentrics.x = 1.f - inBarycentrics.x - inBarycentrics.y;
		barycentrics.y = inBarycentrics.x;
		barycentrics.z = inBarycentrics.y;
	}

	float3 Interpolate(float3 v0, float3 v1, float3 v2)
	{
		return v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
	}

	float2 Interpolate(float2 v0, float2 v1, float2 v2)
	{
		const float u = v0.x * barycentrics.x + v1.x * barycentrics.y + v2.x * barycentrics.z;
		const float v = v0.y * barycentrics.x + v1.y * barycentrics.y + v2.y * barycentrics.z;
		return float2(u, v);
	}

	float3 barycentrics;
};