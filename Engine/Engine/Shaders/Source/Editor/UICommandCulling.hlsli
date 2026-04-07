#pragma once

#define CULLING_GROUP_SIZE_X 16
#define CULLING_GROUP_SIZE_Y 16

#define CULLING_GROUP_SIZE (CULLING_GROUP_SIZE_X * CULLING_GROUP_SIZE_Y)

bool IsAABBIntersectingAABB(float2 aabbMin0, float2 aabbMax0, float2 aabbMin1, float2 aabbMax1)
{
	const float2 pos0 = (aabbMin0 + aabbMax0) * 0.5f;
	const float2 halfSize0 = (aabbMax0 - aabbMin0) * 0.5f;

	const float2 pos1 = (aabbMin1 + aabbMax1) * 0.5f;
	const float2 halfSize1 = (aabbMax1 - aabbMin1) * 0.5f;

	const float dx = pos1.x - pos0.x;
	const float px = (halfSize1.x + halfSize0.x) - abs(dx);

	if (px <= 0.f)
	{
		return false;
	}

	const float dy = pos1.y - pos0.y;
	const float py = (halfSize1.y + halfSize0.y) - abs(dy);

	if (py <= 0.f)
	{
		return false;
	}

	return true;
}