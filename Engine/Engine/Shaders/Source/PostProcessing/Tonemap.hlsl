#include "Vertex.hlsli"
#include "Resources.hlsli"
#include "Utility.hlsli"

#include "Noise.hlsli"
#include "BlueNoise.hlsli"

vt::Tex2D<float3> FinalColor;
vt::Tex2D<float> AverageLuminance;
float MiddleGray;
float WhitePoint;

uint FrameIndex;

struct Output
{
    [[vt::rgba8]] float4 output : SV_Target0;
};

// From https://github.com/bkaradzic/bgfx/blob/master/examples/common/shaderlib.sh
// ---------------------------------------------------------------------------------------
float3 ConvertRGB2XYZ(float3 rgb)
{
	// Reference(s):
	// - RGB/XYZ Matrices
	//   https://web.archive.org/web/20191027010220/http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	float3 xyz;
	xyz.x = dot(float3(0.4124564, 0.3575761, 0.1804375), rgb);
	xyz.y = dot(float3(0.2126729, 0.7151522, 0.0721750), rgb);
	xyz.z = dot(float3(0.0193339, 0.1191920, 0.9503041), rgb);
	return xyz;
}

float3 ConvertXYZ2Yxy(float3 xyz)
{
	// Reference(s):
	// - XYZ to xyY
	//   https://web.archive.org/web/20191027010144/http://www.brucelindbloom.com/index.html?Eqn_XYZ_to_xyY.html
	float inv = 1.0/dot(xyz, float3(1.0, 1.0, 1.0) );
	return float3(xyz.y, xyz.x*inv, xyz.y*inv);
}

float3 ConvertRGB2Yxy(float3 rgb)
{
	return ConvertXYZ2Yxy(ConvertRGB2XYZ(rgb) );
}

float3 ConvertYxy2XYZ(float3 Yxy)
{
	// Reference(s):
	// - xyY to XYZ
	//   https://web.archive.org/web/20191027010036/http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	float3 xyz;
	xyz.x = Yxy.x*Yxy.y/Yxy.z;
	xyz.y = Yxy.x;
	xyz.z = Yxy.x * (1.0 - Yxy.y - Yxy.z) / Yxy.z;
	return xyz;
}

float3 ConvertXYZ2RGB(float3 xyz)
{
	float3 rgb;
	rgb.x = dot(float3( 3.2404542, -1.5371385, -0.4985314), xyz);
	rgb.y = dot(float3(-0.9692660,  1.8760108,  0.0415560), xyz);
	rgb.z = dot(float3( 0.0556434, -0.2040259,  1.0572252), xyz);
	return rgb;
}

float3 ConvertYxy2RGB(float3 Yxy)
{
	return ConvertXYZ2RGB(ConvertYxy2XYZ(Yxy) );
}
// ---------------------------------------------------------------------------------------

float Reinhard2(float x, float whiteSqr)
{
	return (x * (1.f + x / whiteSqr)) / (1.f + x);
}

Output main(FullscreenTriangleVertex input)
{
    float3 pixelColor = FinalColor.Load(int3(input.position.xy, 0));
	//float luminance = constants.averageLuminance.Load(int3(0, 0, 0));
	//
	//float3 Yxy = ConvertRGB2Yxy(pixelColor);
	//
	//float lp = Yxy.x * constants.middleGray / (max(luminance, 0.0001f));
	//Yxy.x = Reinhard2(lp, constants.whitePoint);
	//
	//pixelColor = ConvertYxy2RGB(Yxy);

    float3 dither = RemapPDFTriUnity(BlueNoiseRGBA(input.position.xy, FrameIndex).rgb) / 254.f;

    Output output;
    output.output = float4(LinearToSRGB(pixelColor + dither) , 1.f);
    return output;
} 