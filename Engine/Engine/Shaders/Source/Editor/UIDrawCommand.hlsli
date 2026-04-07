#pragma once

namespace UIPrimitiveType
{
	static const uint Circle = 0;
	static const uint Rect = 1;
	static const uint Line = 2;
	static const uint CircleSegment = 3;
	static const uint TextChar = 4;
	static const uint Image = 5;
}

struct UICommand
{
	uint type;
	int primitiveGroup;

	// Common
	float rotation;
	float scale;

	float4 bounds;

	float2 position;

	float glowDistance;
	float glowStrength;

	float2 shadowOffset;
	float shadowStrength;
	float padding0;

	uint color;
	uint textureIndex;

	// Rounding
	float rounding;

	// Circle
	float radius;

	// Rect
	float2 halfSize;

	// Circle Segment
	float radiusInner;
	float angle;

	// Line
	float2 lineA;
	float2 lineB;

	// Image
	uint2 dimensions;
	float2 padding1;

	// Text
	float4 minMaxUV;
	float4 minMaxPx;
};