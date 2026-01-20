// From https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom

#include "StaticSamplerStates.hlsli"

Texture2D<float3> Source;

#if BLOOM_COMPOSITE
RWTexture2D<float4> RWTarget;
#else
RWTexture2D<float3> RWTarget;
#endif

uint2 TargetResolution;

float FilterRadius;
float BloomStrength;

[numthreads(8, 8, 1)]
void BloomUpsampleCS(uint2 DispatchThreadID : SV_DispatchThreadID)
{
	if (any(DispatchThreadID >= TargetResolution))
	{
		return;
	}

	const float2 texCoords = (float2(DispatchThreadID) + 0.5f) / float2(TargetResolution);
	
	// Take 9 samples around current texel:
	// a - b - c
	// d - e - f
	// g - h - i
	// === ('e' is the current texel) ===
    const float3 a = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - FilterRadius, texCoords.y + FilterRadius), 0.f).rgb;
    const float3 b = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x,				texCoords.y + FilterRadius), 0.f).rgb;
    const float3 c = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + FilterRadius, texCoords.y + FilterRadius), 0.f).rgb;

    const float3 d = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - FilterRadius, texCoords.y), 0.f).rgb;
    const float3 e = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x,				texCoords.y), 0.f).rgb;
    const float3 f = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + FilterRadius, texCoords.y), 0.f).rgb;

    const float3 g = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x - FilterRadius, texCoords.y - FilterRadius), 0.f).rgb;
    const float3 h = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x,				texCoords.y - FilterRadius), 0.f).rgb;
    const float3 i = Source.SampleLevel(StaticBilinearSamplerClamp, float2(texCoords.x + FilterRadius, texCoords.y - FilterRadius), 0.f).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
	//  1   | 1 2 1 |
	// -- * | 2 4 2 |
	// 16   | 1 2 1 |

    float3 result = e * 4.f;
    result += (b + d + f + h) * 2.f;
    result += (a + c + g + i);
    result *= 1.f / 16.f;

#if BLOOM_COMPOSITE
    const float bloomStrength = 0.04f * BloomStrength;

    float4 srcColor = RWTarget[DispatchThreadID];
    srcColor.rgb = lerp(srcColor.rgb, result, bloomStrength);
    
    RWTarget[DispatchThreadID] = srcColor;
#else
    RWTarget[DispatchThreadID] = result;
#endif
}