#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"
#include "GPUScene.hlsli"
#include "BlueNoise.hlsli"
#include "Lights.hlsli"

#include "PathTracingCommon.hlsli"

vt::UniformBuffer<ViewData> View;
vt::UniformBuffer<DirectionalLight> DirectionalLight;
vt::RWTex2D<float4> RWOutputTexture;
uint FrameIndex;

GPUScene GPUSceneData;
BlueNoiseData BlueNoise;

[shader("raygeneration")]
void main()
{
	const ViewData viewData = View.Load();    

	uint3 launchID = DispatchRaysIndex();
	uint3 launchSize = DispatchRaysDimensions();

	Payload payload;
	payload.rngState = launchID.y * launchSize.x + launchID.x;

	float3 summedPixelColor = 0.f;

	const uint SampleCount = 64;
	for (int sample = 0; sample < SampleCount; sample++)
	{
		const float2 pixelCenter = float2(launchID.xy) + float2(StepAndOutputRNGFloat(payload.rngState), StepAndOutputRNGFloat(payload.rngState));
		const float2 inUV = pixelCenter * viewData.invRenderSize.xy;
		float2 d = inUV * 2.0 - 1.0;
	
		float4 target = mul(viewData.inverseProjection, float4(d.x, -d.y, 1, 1));

		float3 rayOrigin = mul(viewData.inverseView, float4(0,0,0,1)).xyz;
		float3 rayDirection = mul(viewData.inverseView, float4(normalize(target.xyz), 0)).xyz;

		float3 accumulatedRayColor = 1.f;

		for (int tracedSegments = 0; tracedSegments < 32; tracedSegments++)
		{
			RayDesc rayDesc;
			rayDesc.Origin = rayOrigin;
			rayDesc.Direction = rayDirection;
			rayDesc.TMin = 0.01f;
			rayDesc.TMax = 10000.f;
			
			TraceRay(g_accelerationStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, rayDesc, payload);

			accumulatedRayColor *= payload.radiance;

			if (payload.miss)
			{
				summedPixelColor += accumulatedRayColor;
				break;
			}
			else
			{
				rayOrigin = payload.rayOrigin;
				rayDirection = payload.rayDirection;
			}
		}
	}

	RWOutputTexture.Store(launchID.xy, float4(summedPixelColor / float(SampleCount), 0.0));
}
