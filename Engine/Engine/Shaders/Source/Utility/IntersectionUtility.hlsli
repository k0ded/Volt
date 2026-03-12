#pragma once

#include "Common.hlsli"

bool RaySphereIntersection(float3 rayOrigin, float3 rayDirection, float4 sphereCenterAndRadius, out float intersectionDistance)
{
	const float sphereRadiusSquared = sphereCenterAndRadius.w * sphereCenterAndRadius.w;

	float3 diff = sphereCenterAndRadius.xyz - rayOrigin;
	float t0 = dot(diff, rayDirection);
	float dSquared = dot(diff, diff) - t0 * t0;
	
	if (dSquared > sphereRadiusSquared)
	{
		intersectionDistance = -1.f;
		return false;
	}

	float t1 = sqrt(sphereRadiusSquared - dSquared);
	intersectionDistance = t0 > t1 + FLT_EPSILON ? t0 - t1 : t0 + t1;
	return intersectionDistance > FLT_EPSILON;
}