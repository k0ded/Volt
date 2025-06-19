#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"

vt::UniformBuffer<ViewData> View;
vt::RWTex2D<float4> OutputTexture;

[numthreads(8, 8, 1)]
void main(uint2 threadId : SV_DispatchThreadID)
{
	const ViewData viewData = View.Load();    

    RayQuery<RAY_FLAG_FORCE_OPAQUE | 
             RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
             RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> query;

	const float2 pixelCenter = float2(threadId.xy) + float2(0.5, 0.5);
	const float2 inUV = pixelCenter * viewData.invRenderSize.xy;
	float2 d = inUV * 2.0 - 1.0;
	
	float4 target = mul(viewData.inverseProjection, float4(d.x, -d.y, 1, 1));

	RayDesc rayDesc;
	rayDesc.Origin = mul(viewData.inverseView, float4(0,0,0,1)).xyz;
	rayDesc.Direction = mul(viewData.inverseView, float4(normalize(target.xyz), 0)).xyz;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;

    query.TraceRayInline(g_accelerationStructure, RAY_FLAG_NONE, 0xFF, rayDesc);

	query.Proceed();

	if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
	{
		OutputTexture.Store(threadId, float4(0.f, 1.f, 0.f, 1.f));
	}
	else
	{
		OutputTexture.Store(threadId, 0.f);
	}
}