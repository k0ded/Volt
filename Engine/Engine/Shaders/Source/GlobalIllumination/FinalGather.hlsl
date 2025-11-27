#include "RayTracing/RayTracingCommon.hlsli"
#include "RayTracing/RayTracingResourceTable.hlsli"
#include "RayTracing/RayTracingTriangleAttributes.hlsli"
#include "RayTracing/RayTracingInline.hlsli"

#include "RenderScene/GPUScene.hlsli"

#include "Utility/Utility.hlsli"
#include "Utility/Packing.hlsli"

#include "GlobalIlluminationCommon.hlsli"
#include "SpatialHashTable.hlsli"

#include "BlueNoise.hlsli"
#include "MonteCarlo.hlsli"

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWIndirectLight;

Texture2D<float4> GBufferAlbedo;
Texture2D<float4> GBufferNormal;
Texture2D<float2> GBufferMaterial;
Texture2D<float> SceneDepth;

RaytracingAccelerationStructure TLAS;

StructuredBuffer<uint> WorldRadianceCacheCellCache;

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

[numthreads(8, 8, 1)]
void FinalGatherCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (any(DispatchThreadID >= View.renderSize))
	{
		return;
	}

	const uint2 pixelPos = DispatchThreadID;
	const float pixelDepth = SceneDepth.Load(int3(pixelPos, 0));

	float4 indirectLight = float4(0.f, 0.f, 0.f, 1.f);

	if (pixelDepth > 0.f)
	{
		const float3 pixelWorldPosition = ReconstructWorldPosition(pixelPos, pixelDepth);
		const float3 pixelNormal = GBufferNormal.Load(int3(pixelPos, 0)).xyz * 2.f - 1.f;
		const float3x3 tangentBasis = GetTangentBasis(pixelNormal);

		uint randomSeed = uint(uint(1973) + (DispatchThreadID.y * View.renderSize.x + DispatchThreadID.x) * uint(9277) + View.frameIndex * uint(26699)) | uint(1);

		const float2 randSample = float2(StepAndOutputRNGFloat(randomSeed), StepAndOutputRNGFloat(randomSeed));
		//const float2 randSample = BlueNoiseVec2(pixelPos, View.frameIndex); //float2(0.f, 0.f);
		float3 raySample = CosineSampleHemisphere(randSample);
		raySample = normalize(mul(raySample, tangentBasis));

		RayDescription rayDesc;
		rayDesc.origin = pixelWorldPosition;
		rayDesc.direction = raySample;
		rayDesc.tMin = 1.f;
		rayDesc.tMax = 100000.f;

		const uint rayFlags = RAY_FLAG_FORCE_OPAQUE | RAY_FLAG_CULL_BACK_FACING_TRIANGLES;
		const uint instanceMask = 0xFF;

		RayTraceInlineResult inlineTraceResult = TraceInlineRay(TLAS, rayFlags, instanceMask, rayDesc);

		if (inlineTraceResult.IsHit() && inlineTraceResult.IsFrontFace())
		{
			const float3 hitPosition = rayDesc.origin + rayDesc.direction * inlineTraceResult.GetHitT();
		
			SpatialHashTable worldRadianceCacheHashTable;

			uint hashIndex;
			if (worldRadianceCacheHashTable.Get(hitPosition, hashIndex))
			{
				indirectLight.rgb = UnpackRGBE(WorldRadianceCacheCellCache[hashIndex]);
			}	
		}
		else
		{
			indirectLight.rgb = SkyColor(raySample);
		}
	}

	RWIndirectLight[pixelPos] = indirectLight;
}