#include "GlobalIlluminationCommon.hlsli"

Texture2D<float3> ProbeAtlas;
Texture2D<float2> ProbeVisibilityAtlas;
Buffer<uint> ProbeStatus;
SamplerState BilinearSampler;

float3 SampleIrradiance(float3 worldPosition, float3 normal)
{
	const float3 dirToCamera = normalize(View.cameraPosition.xyz - worldPosition);

	const float selfShadowBias = 0.3f;
	const float3 biasVector = (normal * 0.2f + dirToCamera * 0.8f) * 0.75f * selfShadowBias;
	const float3 biasedWorldPosition = worldPosition + biasVector;
		
	const uint4 probeCoordsAndCascade = IrradianceVolume::GetProbeCoordsAndCascadeIndexFromWorldPosition(biasedWorldPosition);
	const float3 baseProbeWorldPosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeCoordsAndCascade.xyz, probeCoordsAndCascade.w);

	const float3 alpha = frac(float3(probeCoordsAndCascade.xyz) - 0.5f);

	float3 irradianceSum = 0.f;
	float weightSum = 0.f;

	for (uint i = 0; i < 8; ++i)
	{
		uint3 offset = uint3(i, i >> 1, i >> 2) & 1u;
		uint3 probeGridCoord = clamp(probeCoordsAndCascade.xyz + offset, 0, IrradianceVolumeResolution);

		const float3 probeWorldPosition = IrradianceVolume::GetProbeWorldPositionFromProbeCoords(probeGridCoord, probeCoordsAndCascade.w);
		const uint probeIndex = IrradianceVolume::GetProbeIndexFromProbeCoords(probeGridCoord);

		const uint bitmaskIndex = probeIndex / 32u;
		const uint bitIndex = probeIndex % 32u;

		// Probe is invalid, we'll not sample it.
		if (ProbeStatus[bitmaskIndex] & (1u << bitIndex))
		{
			continue;
		}

		float3 trilinearWeights = max(select(offset > 0, alpha, 1.f - alpha), 0.001f);
		float weight = 1.f;
	
		// "Wrap shading"
#if 0
		{
			const float3 dirToProbe = normalize(probeWorldPosition - worldPosition);
			const float dDotN = (dot(dirToProbe, normal) + 1.f) * 0.5f;
			
			weight *= (dDotN * dDotN) + 0.2f;
		}
#else
		{
			const float3 dirToProbe = normalize(probeWorldPosition - worldPosition);
			weight *= saturate(dot(dirToProbe, normal));		
		}
#endif

		float3 biasedDirToProbe = biasedWorldPosition - probeWorldPosition;
		float distToBiasedPos = length(biasedDirToProbe);
		biasedDirToProbe *= 1.f / distToBiasedPos;

		// Move distance to a larger unit, due to how visibility is stored.
		distToBiasedPos *= 0.01f;

		// Visibility
		{
			const float2 visibilitySamplingUv = IrradianceVolume::GetProbeSamplingUVFromNormal(probeIndex, probeCoordsAndCascade.w, biasedDirToProbe);
			const float2 visibility = ProbeVisibilityAtlas.SampleLevel(BilinearSampler, visibilitySamplingUv, 0);

			float meanDistToOccluder = visibility.x;

			float chebyshevWeight = 1.f;
			if (distToBiasedPos > meanDistToOccluder)
			{
				const float variance = abs((visibility.x * visibility.x) - visibility.y);

				const float distanceDiff = distToBiasedPos - meanDistToOccluder;
				chebyshevWeight = variance / (variance + (distanceDiff * distanceDiff));

				chebyshevWeight = max((chebyshevWeight * chebyshevWeight * chebyshevWeight), 0.f);
			}

			chebyshevWeight = max(0.05f, chebyshevWeight);
			//weight *= chebyshevWeight;
		}

		const float2 samplingUv = IrradianceVolume::GetProbeSamplingUVFromNormal(probeIndex, probeCoordsAndCascade.w, normal);
		const float3 radiance = ProbeAtlas.SampleLevel(BilinearSampler, samplingUv, 0);

		weight *= trilinearWeights.x * trilinearWeights.y * trilinearWeights.z + 0.001f;
		irradianceSum += weight * radiance;
		weightSum += weight;
	}

	return 0.5f * PI * (irradianceSum / weightSum);
}