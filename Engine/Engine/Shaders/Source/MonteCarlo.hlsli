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

// Replace these
// Based on: [Clarberg 2008, "Fast Equal-Area Mapping of the (Hemi)Sphere using SIMD"]
// Fixed sign bit for UV.y == 0 and removed branch before division by using a small epsilon
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
float3 EquiAreaSphericalMapping(float2 UV)
{
	UV = 2 * UV - 1;
	float D = 1 - (abs(UV.x) + abs(UV.y));
	float R = 1 - abs(D);
	// Branch to avoid dividing by 0.
	// Only happens with (0.5, 0.5), usually occurs in odd number resolutions which use the very central texel
	float Phi = R == 0 ? 0 : (PI / 4) * ((abs(UV.y) - abs(UV.x)) / R + 1);
	float F = R * sqrt(2 - R * R);
	return float3(
		F * sign(UV.x) * abs(cos(Phi)),
		F * sign(UV.y) * abs(sin(Phi)),
		sign(D) * (1 - R * R)
	);
}

// Based on: [Clarberg 2008, "Fast Equal-Area Mapping of the (Hemi)Sphere using SIMD"]
// Removed branch before division by using a small epsilon
// https://fileadmin.cs.lth.se/graphics/research/papers/2008/simdmapping/clarberg_simdmapping08_preprint.pdf
float2 InverseEquiAreaSphericalMapping(float3 Direction)
{
	// Most use cases of this func generate Direction by diffing two positions and thus unnormalized
	Direction = normalize(Direction);
	
	float3 AbsDir = abs(Direction);
	float R = sqrt(1 - AbsDir.z);
	float Epsilon = 5.42101086243e-20; // 2^-64 (this avoids 0/0 without changing the rest of the mapping)
	float x = min(AbsDir.x, AbsDir.y) / (max(AbsDir.x, AbsDir.y) + Epsilon);

	// Coefficients for 6th degree minimax approximation of atan(x)*2/pi, x=[0,1].
	const float t1 = 0.406758566246788489601959989e-5f;
	const float t2 = 0.636226545274016134946890922156f;
	const float t3 = 0.61572017898280213493197203466e-2f;
	const float t4 = -0.247333733281268944196501420480f;
	const float t5 = 0.881770664775316294736387951347e-1f;
	const float t6 = 0.419038818029165735901852432784e-1f;
	const float t7 = -0.251390972343483509333252996350e-1f;

	// Polynomial approximation of atan(x)*2/pi
	float Phi = t6 + t7 * x;
	Phi = t5 + Phi * x;
	Phi = t4 + Phi * x;
	Phi = t3 + Phi * x;
	Phi = t2 + Phi * x;
	Phi = t1 + Phi * x;

	Phi = (AbsDir.x < AbsDir.y) ? 1 - Phi : Phi;
	float2 UV = float2(R - Phi * R, Phi * R);
	UV = (Direction.z < 0) ? 1 - UV.yx : UV;
	UV = asfloat(asuint(UV) ^ (asuint(Direction.xy) & 0x80000000u));
	return UV * 0.5 + 0.5;
}