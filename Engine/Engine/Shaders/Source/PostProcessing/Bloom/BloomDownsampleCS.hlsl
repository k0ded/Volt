// From https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#include "StaticSamplerStates.hlsli"

RWTexture2D<float3> RWTarget;
Texture2D<float4> Source;

uint2 SrcResolution;
uint2 TargetResolution;

[numthreads(8, 8, 1)]
void BloomDownsampleCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (any(DispatchThreadID >= TargetResolution))
	{
		return;
	}

	const float2 srcTexelSize = 1.f / float2(SrcResolution);
	const float2 texCoords = (float2(DispatchThreadID) + 0.5f) / float2(TargetResolution);
	
	const float3 a = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - 2.f * srcTexelSize.x, texCoords.y + 2.f * srcTexelSize.y), 0).rgb;
	const float3 b = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x						 , texCoords.y + 2.f * srcTexelSize.y), 0).rgb;
	const float3 c = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + 2.f * srcTexelSize.x, texCoords.y + 2.f * srcTexelSize.y), 0).rgb;

	const float3 d = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - 2.f * srcTexelSize.x, texCoords.y						 ), 0).rgb;
	const float3 e = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x						 , texCoords.y						 ), 0).rgb;
	const float3 f = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + 2.f * srcTexelSize.x, texCoords.y						 ), 0).rgb;

	const float3 g = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - 2.f * srcTexelSize.x, texCoords.y - 2.f * srcTexelSize.y), 0).rgb;
	const float3 h = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x						 , texCoords.y - 2.f * srcTexelSize.y), 0).rgb;
	const float3 i = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + 2.f * srcTexelSize.x, texCoords.y - 2.f * srcTexelSize.y), 0).rgb;

    const float3 j = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - srcTexelSize.x, texCoords.y + srcTexelSize.y), 0).rgb;
    const float3 k = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + srcTexelSize.x, texCoords.y + srcTexelSize.y), 0).rgb;
    const float3 l = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - srcTexelSize.x, texCoords.y - srcTexelSize.y), 0).rgb;
    const float3 m = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + srcTexelSize.x, texCoords.y - srcTexelSize.y), 0).rgb;

	// Apply weighted distribution:
	// 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
	// a,b,d,e * 0.125
	// b,c,e,f * 0.125
	// d,e,g,h * 0.125
	// e,f,h,i * 0.125
	// j,k,l,m * 0.5
	// This shows 5 square areas that are being sampled. But some of them overlap,
	// so to have an energy preserving downsample we need to make some adjustments.
	// The weights are the distributed, so that the sum of j,k,l,m (e.g.)
	// contribute 0.5 to the final color output. The code below is written
	// to effectively yield this sum. We get:
	// 0.125*5 + 0.03125*4 + 0.0625*4 = 1

    float3 result = e * 0.125f;
    result += (a + c + g + i) * 0.03125f;
    result += (b + d + f + h) * 0.0625f;
    result += (j + k + l + m) * 0.125;

    result = max(result, 0.0001f);
    
	RWTarget[DispatchThreadID] = result;
}