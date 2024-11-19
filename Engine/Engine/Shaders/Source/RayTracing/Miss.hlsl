#include "PathTracingCommon.hlsli"

float3 SkyColor(float3 direction)
{
	if (direction.y > 0.f)
	{
		return lerp(1.f, float3(0.25f, 0.5f, 1.f), direction.y);
	}
	else	
	{
		return 0.03f;
	}
}

[shader("miss")]
void main(inout Payload p)
{
    p.radiance = SkyColor(WorldRayDirection());
	p.miss = true;
}