#include "HBAOConstants.hlsli"

Texture2D<float2> SrcTexture;
SamplerState PointClampSampler;
SamplerState BilinearClampSampler;

#if X_BLUR
RWTexture2D<float2> DstTexture;
#else
RWTexture2D<float> DstTexture;
#endif

float2 SampleAOZ(float2 uv)
{
    return SrcTexture.SampleLevel(PointClampSampler, uv, 0);
}

float2 SampleAOZLinear(float2 uv)
{
    return SrcTexture.SampleLevel(BilinearClampSampler, uv, 0);
}

#define KERNEL_RADIUS 6
#define USE_DEPTH_SLOPE 1

struct PixelData 
{
	float2 UV;
    float Depth;
    float Sharpness;
    float Scale;
    float Bias;
};

float CrossBilateralWeight(float r, float z, float depthSlope, PixelData pixelData)
{
	const float BlurSigma = (KERNEL_RADIUS + 1.0f) * 0.5f;
	const float BlurFalloff = 1.0f / (2.0f*BlurSigma*BlurSigma);

#if USE_DEPTH_SLOPE
	z -= depthSlope * r;
#endif
	
    float DeltaZ = z * pixelData.Scale + pixelData.Bias;
	return exp2(-r * r * BlurFalloff - DeltaZ*DeltaZ);
}

void BlurAO(inout float total_ao, inout float total_weight, float R, float depthSlope, PixelData pixelData, float2 aoz)
{
	float w = CrossBilateralWeight(R, aoz.y, depthSlope, pixelData);
	total_ao += aoz.x * w;
	total_weight += w;
}

void ProcessRadius(
	float2 deltaUV,
	PixelData pixel,
	inout float total_ao,
	inout float total_w
)
{
	float i = 1;
	float depthSlope = 0;
#if USE_DEPTH_SLOPE
	float2 aoz = SampleAOZ(pixel.UV + deltaUV);
	depthSlope = aoz.y - pixel.Depth;
	BlurAO(total_ao, total_w, 1, depthSlope, pixel, aoz);
	i++;
#endif

	[unroll]
	for(; i <= KERNEL_RADIUS/2; i += 1.0)
	{
		aoz = SampleAOZ(pixel.UV + i * deltaUV);
		BlurAO(total_ao, total_w, i, depthSlope, pixel, aoz);
	}

    [unroll(KERNEL_RADIUS/4)]
	for(; i <= KERNEL_RADIUS; i += 2.0)
	{
		aoz = SampleAOZLinear(pixel.UV + (i + 0.5) * deltaUV);
		BlurAO(total_ao, total_w, i, depthSlope, pixel, aoz);
	}
}

[shader("compute")]
[numthreads(8, 8, 1)]
void MainCS(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    float2 uv = (float2(dispatchThreadID) + 0.5f) * constants.InvFullSize;

    if (uv.x >= 1 || uv.y >= 1)
        return;

    float2 centerSample = SampleAOZ(uv);
	PixelData data;
    data.UV = uv;
	data.Depth = centerSample.y;
	data.Sharpness = 0.1;
    data.Scale = data.Sharpness;
    data.Bias = -data.Depth * data.Sharpness;

	float total_ao = centerSample.x;
	float total_weight = 1;

#if X_BLUR
	float2 deltaUV = float2(constants.InvFullSize.x, 0);
#else
	float2 deltaUV = float2(0, constants.InvFullSize.y);
#endif

    ProcessRadius(deltaUV, data, total_ao, total_weight);
    ProcessRadius(-deltaUV, data, total_ao, total_weight);

	float AO = total_ao / total_weight;

#if X_BLUR
    DstTexture[dispatchThreadID] = float2(AO, centerSample.y);
#else
	DstTexture[dispatchThreadID] = pow(AO, constants.power);
#endif
}