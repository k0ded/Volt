#include "Resources.hlsli"
#include "Structures.hlsli"
#include "RayTracing.hlsli"
#include "GPUScene.hlsli"
#include "VisibilityBuffer.hlsli"
#include "MathConstants.hlsli"

struct Barycentrics
{
	float2 bary;
};

struct ObjectHitInfo
{
	float3 color;
	float3 worldPosition;
	float3 worldNormal;
};

struct Constants
{
    vt::UniformBuffer<ViewData> viewData;
    vt::RWTex2D<float4> outputTexture;

    GPUScene gpuScene;
};

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

float3 GetInterpolatedFloat3(in float3 values[3], float3 barycentrics)
{
	return values[0] * barycentrics.x + values[1] * barycentrics.y + values[2] * barycentrics.z;
}

ObjectHitInfo GetObjectHitInfo(in const GPUScene gpuScene, in float2 barycentrics, uint instanceId, uint primitiveIndex)
{
	const PrimitiveDrawData primitiveDrawData = gpuScene.primitiveDrawDataBuffer.Load(instanceId);
	const GPUMesh mesh = gpuScene.meshesBuffer.Load(primitiveDrawData.meshId);

	const uint3 triIndices = uint3(mesh.indexBuffer.Load(primitiveIndex * 3), mesh.indexBuffer.Load(primitiveIndex * 3 + 1), mesh.indexBuffer.Load(primitiveIndex * 3 + 2));
    const float3 barycentricCoords = float3(1.0f - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);

	const PositionData vertexPositions = LoadVertexPositions(mesh.vertexPositionsBuffer, triIndices);
    const MaterialData materialData = LoadVertexMaterialData(mesh.vertexMaterialBuffer, triIndices);

	ObjectHitInfo result;
	result.worldPosition = primitiveDrawData.transform.GetWorldPosition(GetInterpolatedFloat3(vertexPositions.positions, barycentricCoords));
	result.worldNormal = primitiveDrawData.transform.RotateVector(GetInterpolatedFloat3(materialData.normals, barycentricCoords));
	result.color = 0.7f;

	return result;
}

// Random number generation using pcg32i_random_t, using inc = 1. Our random state is a uint.
uint StepRNG(uint rngState)
{
  return rngState * 747796405 + 1;
}

// Steps the RNG and returns a floating-point value between 0 and 1 inclusive.
float StepAndOutputRNGFloat(inout uint rngState)
{
  // Condensed version of pcg_output_rxs_m_xs_32_32, with simple conversion to floating-point [0,1].
  rngState  = StepRNG(rngState);
  uint word = ((rngState >> ((rngState >> 28) + 4)) ^ rngState) * 277803737;
  word      = (word >> 22) ^ word;
  return float(word) / 4294967295.0f;
}

[numthreads(8, 8, 1)]
void main(uint2 threadId : SV_DispatchThreadID)
{
    const Constants constants = GetConstants<Constants>();
	const ViewData viewData = constants.viewData.Load(); 

	if (any(threadId >= uint2(viewData.renderSize)))
	{
		return;
	}

	uint rngState = viewData.renderSize.x * threadId.y + threadId.x;
	float3 summedPixelColor = 0.f;

	const uint SampleCount = 64;
	for (int sample = 0; sample < SampleCount; sample++)
	{
		const float2 pixelCenter = float2(threadId.xy) + float2(StepAndOutputRNGFloat(rngState), StepAndOutputRNGFloat(rngState));
		const float2 inUV = pixelCenter * viewData.invRenderSize.xy;
		float2 d = inUV * 2.0 - 1.0;
	
		float4 target = mul(viewData.inverseProjection, float4(d.x, -d.y, 1, 1));

		float3 rayOrigin = mul(viewData.inverseView, float4(0,0,0,1)).xyz;
		float3 rayDirection = mul(viewData.inverseView, float4(normalize(target.xyz), 0)).xyz;

		float3 accumulatedRayColor = 1.f;

		for (int tracedSegments = 0; tracedSegments < 32; tracedSegments++)
		{
			RayQuery<RAY_FLAG_FORCE_OPAQUE | 
			         RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;

			RayDesc rayDesc;
			rayDesc.Origin = rayOrigin;
			rayDesc.Direction = rayDirection;
			rayDesc.TMin = 0.f;
			rayDesc.TMax = 10000.0;

			query.TraceRayInline(g_accelerationStructure, RAY_FLAG_NONE, 0xFF, rayDesc);
			query.Proceed();

			if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
			{
				ObjectHitInfo objectHitInfo = GetObjectHitInfo(constants.gpuScene, query.CommittedTriangleBarycentrics(), query.CommittedInstanceID(), query.CommittedPrimitiveIndex());
				accumulatedRayColor *= objectHitInfo.color;				
				
				objectHitInfo.worldNormal = faceforward(objectHitInfo.worldNormal, rayDirection, objectHitInfo.worldNormal);

				rayOrigin = objectHitInfo.worldPosition + 0.01f * objectHitInfo.worldNormal;

				
				const float theta = 2.f * PI * StepAndOutputRNGFloat(rngState);
				const float u = 2.f * StepAndOutputRNGFloat(rngState) - 1.f;
				const float r = sqrt(1.f - u * u);
				
				rayDirection = objectHitInfo.worldNormal + float3(r * cos(theta), r * sin(theta), u);
				rayDirection = normalize(rayDirection);
			}
			else
			{
				summedPixelColor += accumulatedRayColor * SkyColor(rayDirection);
				break;
			}
		}
	}

	constants.outputTexture.Store(threadId, float4(summedPixelColor / float(SampleCount), 1.f));
}