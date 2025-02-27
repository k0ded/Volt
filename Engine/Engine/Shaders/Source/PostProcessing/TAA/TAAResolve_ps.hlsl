#include "Vertex.hlsli"
#include "Resources.hlsli"

struct Constants
{
    vt::Tex2D<float3> currentColor;
    vt::Tex2D<float3> previousColor;
    vt::Tex2D<float> sceneDepth;

    vt::Tex2D<float2> velocityTexture;

    vt::TextureSampler linearSampler;
    uint2 renderSize;
    uint frameIndex;
};

struct Output
{
    [[vt::r11f_g11f_b10f]] float3 output : SV_Target0;
    [[vt::r11f_g11f_b10f]] float3 accumulation : SV_Target1;
};

static const float FLT_EPS = 0.00000001f;

float FilterCubic(float x, float B, float C)
{
    float y = 0.0f;
    float x2 = x * x;
    float x3 = x * x * x;
    if(x < 1)
        y = (12 - 9 * B - 6 * C) * x3 + (-18 + 12 * B + 6 * C) * x2 + (6 - 2 * B);
    else if (x <= 2)
        y = (-B - 6 * C) * x3 + (6 * B + 30 * C) * x2 + (-12 * B - 48 * C) * x + (8 * B + 24 * C);

    return y / 6.0f;
}

float FilterMitchell(float x)
{
    float cubicX = x * 2.f;
    return FilterCubic(cubicX, 1 / 3.f, 1 / 3.f);
}

// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float3 SampleTextureCatmullRom(in vt::Tex2D<float3> tex, in vt::TextureSampler linearSampler, in float2 uv, in float2 texSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv * texSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 /= texSize;
    texPos3 /= texSize;
    texPos12 /= texSize;

    float3 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return result;
}

// Playdead clip function
float4 clip_aabb(float3 aabb_min, float3 aabb_max, float4 p, float4 q)
{
#if 1
	// note: only clips towards aabb center (but fast!)
	float3 p_clip = 0.5 * (aabb_max + aabb_min);
	float3 e_clip = 0.5 * (aabb_max - aabb_min) + FLT_EPS;

	float4 v_clip = q - float4(p_clip, p.w);
	float3 v_unit = v_clip.xyz / e_clip;
	float3 a_unit = abs(v_unit);
	float ma_unit = max(a_unit.x, max(a_unit.y, a_unit.z));

	if (ma_unit > 1.0)
		return float4(p_clip, p.w) + v_clip / ma_unit;
	else
		return q;// point inside aabb
#else
	float4 r = q - p;
	float3 rmax = aabb_max - p.xyz;
	float3 rmin = aabb_min - p.xyz;

	const float eps = FLT_EPS;

	if (r.x > rmax.x + eps)
		r *= (rmax.x / r.x);
	if (r.y > rmax.y + eps)
		r *= (rmax.y / r.y);
	if (r.z > rmax.z + eps)
		r *= (rmax.z / r.z);

	if (r.x < rmin.x - eps)
		r *= (rmin.x / r.x);
	if (r.y < rmin.y - eps)
		r *= (rmin.y / r.y);
	if (r.z < rmin.z - eps)
		r *= (rmin.z / r.z);

	return p + r;
#endif
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2127, 0.7152, 0.0722));
}

Output main(FullscreenTriangleVertex input)
{
    const Constants constants = GetConstants<Constants>();

    float3 sourceSampleTotal = 0.f;
    float sourceSampleWeight = 0.f;
    
    float3 neighbourhoodMin = 10000.f;
    float3 neighbourhoodMax = -10000.f;
    
    float3 m1 = 0.f;
    float3 m2 = 0.f;
    
    float closestDepth = 0.f;
    int2 closestDepthPixelPosition = 0;
    
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            int2 pixelPosition = input.position.xy + int2(x, y);
            pixelPosition = clamp(pixelPosition, 0, constants.renderSize - 1);
    
            float3 neighbour = max(0.f, constants.currentColor.Load(int3(pixelPosition, 0)));
            float subSampleDistance = length(float2(x, y));
            float subSampleWeight = FilterMitchell(subSampleDistance);
    
            sourceSampleTotal += neighbour * subSampleWeight;
            sourceSampleWeight += subSampleWeight;
    
            neighbourhoodMin = min(neighbourhoodMin, neighbour);
            neighbourhoodMax = max(neighbourhoodMax, neighbour);
    
            m1 += neighbour;
            m2 += neighbour * neighbour;
    
            float currentDepth = constants.sceneDepth.Load(int3(pixelPosition, 0));
            if (currentDepth > closestDepth)
            {
                closestDepth = currentDepth;
                closestDepthPixelPosition = pixelPosition;
            }
        } 
    }

    float3 sourceSample = sourceSampleTotal / sourceSampleWeight;

    float2 motionVector = constants.velocityTexture.Load(int3(closestDepthPixelPosition, 0));
    float2 historyTexCoord = input.uv + motionVector;
    
    if (any(historyTexCoord != saturate(historyTexCoord)) || constants.frameIndex == 0)
    {
        Output output;
        output.output = sourceSample;
        output.accumulation = sourceSample;
        return output;
    }
    
    float3 historySample = SampleTextureCatmullRom(constants.previousColor, constants.linearSampler, historyTexCoord, float2(constants.renderSize));
    
    const float oneOverSampleCount = 1.f / 9.f;
    const float gamma = 1.f;
    float3 mu = m1 * oneOverSampleCount;
    float3 sigma = sqrt(abs((m2 * oneOverSampleCount) - (mu * mu)));
    float3 minc = mu - gamma * sigma;
    float3 maxc = mu + gamma * sigma;
    
    historySample = clip_aabb(minc, maxc, float4(clamp(historySample, neighbourhoodMin, neighbourhoodMax), 1.f), float4(historySample, 1.f)).rgb;
    
    float sourceWeight = 0.05f;
    float historyWeight = 1.f - sourceWeight;
       
    float3 compressedSource = sourceSample * rcp(max(max(sourceSample.r, sourceSample.g), sourceSample.b) + 1.f);
    float3 compressedHistory = historySample * rcp(max(max(historySample.r, historySample.g), historySample.b) + 1.f);
    
    float luminanceSource = Luminance(compressedSource);
    float luminanceHistory = Luminance(compressedHistory);
    
    sourceWeight *= 1.f / (1.f + luminanceSource);
    historyWeight *= 1.f / (1.f + luminanceHistory);
    
    float3 result = (sourceSample * sourceWeight + historySample * historyWeight) / max(sourceWeight + historyWeight, FLT_EPS);
    
    Output output;
    output.output = result;
    output.accumulation = result;

    return output;
}