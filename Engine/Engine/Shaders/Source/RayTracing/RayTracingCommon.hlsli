struct RayDescription
{
	float3 origin;
	float tMin;
	float3 direction;
	float tMax;

	RayDesc GetNativeDesc()
	{
		RayDesc desc;
		desc.Origin = origin;
		desc.TMin = tMin;
		desc.Direction = direction;
		desc.TMax = tMax;
	
		return desc;
	}
};