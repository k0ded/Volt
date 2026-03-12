#pragma once

struct Payload
{
	float3 radiance;
	float3 rayOrigin;
	float3 rayDirection;
    uint rngState;
	bool miss;
};

struct ShadowPayload
{
    float visibility;
};

float3 GetInterpolatedFloat3(in float3 values[3], float3 barycentrics)
{
	return values[0] * barycentrics.x + values[1] * barycentrics.y + values[2] * barycentrics.z;
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