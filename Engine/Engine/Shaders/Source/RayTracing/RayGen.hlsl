#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"

struct Payload
{
	float3 hitValue;
};

struct Constants
{
    vt::UniformBuffer<ViewData> viewData;
    vt::RWTex2D<float4> outputTexture;
};

[shader("raygeneration")]
void main()
{
    const Constants constants = GetConstants<Constants>();
	const ViewData viewData = constants.viewData.Load();    

	uint3 launchID = DispatchRaysIndex();
	uint3 launchSize = DispatchRaysDimensions();

	const float2 pixelCenter = float2(launchID.xy) + float2(0.5, 0.5);
	const float2 inUV = pixelCenter * viewData.invRenderSize.xy;
	float2 d = inUV * 2.0 - 1.0;
	
	float4 target = mul(viewData.inverseProjection, float4(d.x, -d.y, 1, 1));

	RayDesc rayDesc;
	rayDesc.Origin = mul(viewData.inverseView, float4(0,0,0,1)).xyz;
	rayDesc.Direction = mul(viewData.inverseView, float4(normalize(target.xyz), 0)).xyz;
	rayDesc.TMin = 0.001;
	rayDesc.TMax = 10000.0;

	Payload payload;
	TraceRay(g_accelerationStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

	constants.outputTexture.Store(launchID.xy, float4(payload.hitValue, 0.0));
}
