#include "RenderScene/GPUScene.hlsli"
#include "Utility/Matrix.hlsli"

float4x4 GetSkinningMatrix(uint boneOffset, uint4 influences, float4 weights)
{
	float4x4 result = 0.f;

	result += mul(AnimatedBones[boneOffset + influences[0]], weights[0]);
	result += mul(AnimatedBones[boneOffset + influences[1]], weights[1]);
	result += mul(AnimatedBones[boneOffset + influences[2]], weights[2]);
	result += mul(AnimatedBones[boneOffset + influences[3]], weights[3]);

	return result;
}