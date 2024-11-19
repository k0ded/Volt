#pragma once

#include "MathConstants.hlsli"

// [Frisvad 2012, "Building an Orthonormal Basis from a 3D Unit Vector Without Normalization"]
// Discontinuity at TangentZ.z < -0.9999999f
float3x3 GetTangentBasis( float3 tangentZ )
{
	const float sign = tangentZ.z >= 0 ? 1 : -1;
	const float a = -rcp( sign + tangentZ.z );
	const float b = tangentZ.x * tangentZ.y * a;
	
	float3 TangentX = { 1 + sign * a * (tangentZ.x * tangentZ.x), sign * b, -sign * tangentZ.x };
	float3 TangentY = { b,  sign + a * (tangentZ.y * tangentZ.y), -tangentZ.y };

	return float3x3( TangentX, TangentY, tangentZ );
}

// PDF = 1 / (2 * PI)
float4 UniformSampleHemisphere( float2 e )
{
	float phi = 2 * PI * e.x;
	float cosTheta = e.y;
	float sinTheta = sqrt( 1 - cosTheta * cosTheta );

	float3 h;
	h.x = sinTheta * cos( phi );
	h.y = sinTheta * sin( phi );
	h.z = cosTheta;

	float pdf = 1.0 / (2 * PI);

	return float4( h, pdf );
}

// PDF = NoL / PI
float3 CosineSampleHemisphere( float2 e )
{
    float3 dir;
    float r = sqrt(e.x);
    float phi = PI * 2 * e.y;
    dir.x = r * cos(phi);
    dir.y = r * sin(phi);
    dir.z = sqrt(max(0.0, 1.0 - dir.x * dir.x - dir.y * dir.y));
    return dir;
}

// Returns a point on the unit circle and a radius in z
float3 ConcentricDiskSamplingHelper(float2 e)
{
	// Rescale input from [0,1) to (-1,1). This ensures the output radius is in [0,1)
	float2 p = 2 * e - 0.99999994;
	float2 a = abs(p);
	float Lo = min(a.x, a.y);
	float Hi = max(a.x, a.y);
	float epsilon = 5.42101086243e-20; // 2^-64 (this avoids 0/0 without changing the rest of the mapping)
	float phi = (PI / 4) * (Lo / (Hi + epsilon) + 2 * float(a.y >= a.x));
	float radius = Hi;
	// copy sign bits from p
	const uint signMask = 0x80000000;
	float2 Disk = asfloat((asuint(float2(cos(phi), sin(phi))) & ~signMask) | (asuint(p) & signMask));
	// return point on the circle as well as the radius
	return float3(Disk, radius);
}

float2 UniformSampleDiskConcentric( float2 e )
{
	float3 result = ConcentricDiskSamplingHelper(e);
	return result.xy * result.z; // uniform sampling
}