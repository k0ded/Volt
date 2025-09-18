#pragma once

#include "Utility.hlsli"

float3 OctNormalDecode(float2 f)
{
    f = f * 2.0 - 1.0;
 
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = clamp(-n.z, 0.f, 1.f);
	
    n.x += n.x >= 0.0f ? -t : t;
    n.y += n.y >= 0.0f ? -t : t;

    return normalize(n);
}

float2 decode_diamond(float p)
{
    float2 v;

    // Remap p to the appropriate segment on the diamond
    float p_sign = sign(p - 0.5f);
    v.x = -p_sign * 4.f * p + 1.f + p_sign * 2.f;
    v.y = p_sign * (1.f - abs(v.x));

    // Normalization extends the point on the diamond back to the unit circle
    return normalize(v);
}

float3 decode_tangent(float3 normal, float diamond_tangent)
{
    // As in the encode step, find our canonical tangent basis span(t1, t2)
    float3 t1;
    if (abs(normal.y) > abs(normal.z))
    {
        t1 = float3(normal.y, -normal.x, 0.f);
    }
    else
    {
        t1 = float3(normal.z, 0.f, -normal.x);
    }
    t1 = normalize(t1);

    float3 t2 = cross(t1, normal);

    // Recover the coordinates used with t1 and t2
    float2 packed_tangent = decode_diamond(diamond_tangent);

    return packed_tangent.x * t1 + packed_tangent.y * t2;
}

const float3 DecodeNormal(uint normalInt)
{
    uint2 octIntNormal;
    
    octIntNormal.x = normalInt & 0xFF;
    octIntNormal.y = (normalInt >> 8) & 0xFF;
    
    float2 octNormal = 0.f;
    octNormal.x = float(octIntNormal.x) / 255.f;
    octNormal.y = float(octIntNormal.y) / 255.f;
    
    return OctNormalDecode(octNormal);
}

const float3 DecodeTangent(float3 normal, float tangentFloat)
{
    return decode_tangent(normal, tangentFloat);
}

float4 UnpackUIntToFloat4(uint packedValue)
{
    float4 result = 0.f;
    
    // Extract the components from the packed uint
    uint packedX = (packedValue >> 24) & 0xFF;
    uint packedY = (packedValue >> 16) & 0xFF;
    uint packedZ = (packedValue >> 8) & 0xFF;
    uint packedW = packedValue & 0xFF;

    // Convert packed components back to float range
    result.x = float(packedX) / 255.0f;
    result.y = float(packedY) / 255.0f;
    result.z = float(packedZ) / 255.0f;
    result.w = float(packedW) / 255.0f;
    
    return result;
}

float2 UnpackHalf2FromUInt(uint packedValue)
{
    return float2(
        f16tof32((packedValue >> 16u) & 0xFFFF),
        f16tof32(packedValue & 0xFFFF)
    );
}

float2 UnitVectorToOctahedron(float3 n)
{
	n.xy /= dot(1, abs(n));
	if( n.z <= 0 )
	{
		n.xy = (1 - abs(n.yx)) * select(n.xy >= 0, float2(1,1), float2(-1,-1));
	}
	return n.xy;
}

float3 OctahedronToUnitVector(float2 oct)
{
	float3 n = float3( oct, 1 - dot( 1, abs(oct) ) );
	float t = max( -n.z, 0 );
	n.xy += select(n.xy >= 0, float2(-t, -t), float2(t, t));
	return normalize(n);
}

// Pack normal vector into an uint32 using Octahedron encoding
uint PackNormalToUInt32(float3 normal)
{
	float2 normalAsOctahedron = UnitVectorToOctahedron(normal);
	uint2 quantizedOctahedron = clamp((normalAsOctahedron * 0.5 + 0.5) * 65535.0 + 0.5, 0.0, 65535.0);
	return ((quantizedOctahedron.x & 0xFFFF) << 16) | (quantizedOctahedron.y & 0xFFFF);
}

// Pack normal vector into from an uint32 using Octahedron encoding
float3 UnpackNormalFromUInt32(uint packedNormal)
{
	uint2 quantizedOctahedron = uint2((packedNormal >> 16) & 0xFFFF, packedNormal & 0xFFFF);
	float2 worldNormalAsOctahedron = ((quantizedOctahedron / 65535.0) - 0.5) * 2.0;
	float3 worldNormal = OctahedronToUnitVector(worldNormalAsOctahedron);
	return worldNormal;
}