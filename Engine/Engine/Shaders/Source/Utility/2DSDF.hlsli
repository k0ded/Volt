#pragma once

#include "MathConstants.hlsli"

float SDF_Circle(float2 pixelPos, float radius)
{
	return length(pixelPos) - radius;
}

float SDF_Rectangle(float2 pixelPos, float2 halfSize, float rounding)
{
	float2 compWiseEdgeDistance = abs(pixelPos) - halfSize + rounding;
	float outsideDistance = length(max(compWiseEdgeDistance, 0.f)) - rounding;
	float insideDistance = min(max(compWiseEdgeDistance.x, compWiseEdgeDistance.y), 0.f);

	return outsideDistance + insideDistance;
}

float SDF_Line(float2 pixelPos, float2 posA, float2 posB, float radius)
{
	float2 ba = posA - posB;
	float2 pa = pixelPos - posA;

	float h = clamp(dot(pa, ba) / dot(ba, ba), 0.f, 1.f);
	return length(pa - h * ba) - radius;
}

float SDF_CircleSegment(float2 pixelPos, float angle, float innerRadius, float outerRadius)
{
	float radius = (outerRadius + innerRadius) * 0.5f;
	float thickness = (outerRadius - innerRadius) * 0.5f;

	const float2 n = float2(cos(angle), sin(angle));

	pixelPos.x = abs(pixelPos.x);
	pixelPos = mul(pixelPos, float2x2(n.x, n.y, -n.y, n.x));
	
	const float d = length(pixelPos) - radius;

	return max(
		abs(d) - thickness,
		length(float2(pixelPos.x, max(0.f, abs(-pixelPos.y + radius) - thickness))) * sign(pixelPos.x)
	);
}

float2 SDF_Scale(float2 pos, float scale)
{
	const float rcpScale = rcp(scale);
	return pos * rcpScale;
}

float2 SDF_Rotate(float2 pos, float rotation)
{
	const float angle = rotation * PI * 2.f * -1.f;
	float sine, cosine;
	sincos(angle, sine, cosine);

	return float2(cosine * pos.x + sine * pos.y, cosine * pos.y - sine * pos.x);
}

float2 SDF_Translate(float2 posA, float2 posB)
{
	return posA - posB;
}

float2 SDF_Transform(float2 position, float scale, float rotation, float2 translation)
{
	position = SDF_Rotate(position, rotation);
	position = SDF_Scale(position, scale);
	position = SDF_Translate(position, translation);

	return position;
}

float SDF_Merge(float sdf0, float sdf1)
{
	return min(sdf0, sdf1);
}

float SDF_Intersection(float sdf0, float sdf1)
{
	return max(sdf0, sdf1);
}

float SDF_Subtract(float sdf0, float sdf1)
{
	return SDF_Intersection(sdf0, -sdf1);
}

float SDF_Interpolate(float sdf0, float sdf1, float t)
{
	return lerp(sdf0, sdf1, t);
}

float SDF_MakeHollow(float sdf, float radius)
{
	return abs(sdf + radius) - radius;
}
