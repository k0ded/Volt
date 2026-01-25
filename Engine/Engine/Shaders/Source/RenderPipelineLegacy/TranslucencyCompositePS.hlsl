#include "Common.hlsli"

#include "Utility/FullscreenTriangleVertex.hlsli"

Texture2D<float4> Accumulation;
Texture2D<float> Revealage;

bool IsApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * FLT_EPSILON;
}

float max3(float3 val)
{
    return max(max(val.r, val.g), val.b);
}

struct Output
{
    [[vt::rgba16f]] float4 output : SV_Target0;
};

Output MainPS(FullscreenTriangleVertex input)
{
	const uint2 pixelCoords = uint2(input.position.xy);

	const float revealage = Revealage.Load(int3(pixelCoords, 0));
	if (IsApproximatelyEqual(revealage, 1.f))
	{
		discard;
	}

	float4 accumulation = Accumulation.Load(int3(pixelCoords, 0));

	if (isinf(max3(accumulation.rgb)))
	{
		accumulation.rgb = accumulation.a;
	}

	float3 avgColor = accumulation.rgb / max(accumulation.a, FLT_EPSILON);

	Output output;
	output.output = float4(avgColor, 1.f - revealage);

	return output;
}