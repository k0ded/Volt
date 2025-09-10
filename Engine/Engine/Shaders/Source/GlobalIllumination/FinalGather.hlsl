#include "RayTracing/RayTracingCommon.hlsli"
#include "RayTracing/RayTracingResourceTable.hlsli"
#include "RayTracing/RayTracingTriangleAttributes.hlsli"

#include "RenderScene/GPUScene.hlsli"

#include "PBR/BRDF.hlsli"
#include "PBR/LightEvaluation.hlsli"

#include "Utility/Utility.hlsli"

#include "BlueNoise.hlsli"
#include "MonteCarlo.hlsli"

VT_SPECIFY_FORMAT("rgba16f")
RWTexture2D<float4> RWIndirectLight;

Texture2D<float4> GBufferAlbedo;
Texture2D<float4> GBufferNormal;
Texture2D<float2> GBufferMaterial;
Texture2D<float> SceneDepth;

RaytracingAccelerationStructure TLAS;

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

		uint randomSeed = DispatchThreadID.x * View.renderSize.x + DispatchThreadID.y + View.frameIndex;

		const float2 randSample = float2(StepAndOutputRNGFloat(randomSeed), StepAndOutputRNGFloat(randomSeed));
		//const float2 randSample = BlueNoiseVec2(pixelPos, View.frameIndex); //float2(0.f, 0.f);
		float3 raySample = CosineSampleHemisphere(randSample);
		raySample = mul(raySample, tangentBasis);

		RayDescription rayDesc;
		rayDesc.origin = pixelWorldPosition;
		rayDesc.direction = raySample;
		rayDesc.tMin = 10.f;
		rayDesc.tMax = 100000.f;

		RayQuery<RAY_FLAG_FORCE_OPAQUE> query;
		query.TraceRayInline(TLAS, 0u, 0xFF, rayDesc.GetNativeDesc());
		query.Proceed();

		if (query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
		{
			const float3 hitPosition = rayDesc.origin + rayDesc.direction * query.CommittedRayT();
			const uint primitiveIndex = query.CommittedInstanceID();

			const PrimitiveDrawData primitiveData = PrimitiveDrawDataBuffer[primitiveIndex];
			const GPUMesh gpuMesh = GPUMeshes[primitiveData.meshId];

			Barycentrics barycentrics;
			barycentrics.Initialize(query.CommittedTriangleBarycentrics());

			TriangleAttributes triangleAttribs = LoadTriangleAttributes(gpuMesh, barycentrics, query.CommittedPrimitiveIndex());

			const float3 albedo = 0.8f;
			const float metallic = 0.f;
			const float roughness = 0.8f;

			BRDFInput brdfInput;
			brdfInput.V = -raySample;
			brdfInput.N = triangleAttribs.normal;
			brdfInput.diffuseColor = CalculateDiffuseColor(albedo, metallic);
			brdfInput.f0 = CalculateF0(albedo, metallic);
			brdfInput.f90 = CalculateF90(albedo, metallic);
			brdfInput.roughness = roughness;
			brdfInput.metalness = metallic;

			// Evaluate lights
			for (uint i = 0; i < View.lightCount; ++i)
			{
				const LightDrawData light = SceneLights[i];
				if (light.lightType == SceneLightType::SLT_Point)
				{
				    indirectLight.rgb += EvaluatePointLight(light, brdfInput, triangleAttribs.position);
				}
				else if (light.lightType == SceneLightType::SLT_Spot)
				{
				    indirectLight.rgb += EvaluateSpotLight(light, brdfInput, triangleAttribs.position);
				}
				else if (light.lightType == SceneLightType::SLT_Directional)
				{
				    indirectLight.rgb += EvaluateDirectionalLight(light, brdfInput, triangleAttribs.position);
				}
				//else if (light.lightType == SceneLightType::SLT_Sky)
				//{
				//    indirectLight.rgb += EvaluateIBL(brdfInput, light);
				//}
			}
		}
	}

	RWIndirectLight[pixelPos] = indirectLight;
}