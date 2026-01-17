#pragma once

#include <RHIModule/Core/RHICommon.h>

namespace Volt::RHI
{
	struct StaticSamplerDeclaration
	{
		TextureFilter minFilter;
		TextureFilter magFilter;
		TextureFilter mipFilter;
		TextureWrap wrapMode;
	
		CompareOperator compareOperator = CompareOperator::None;
		AnisotropyLevel anisotropyLevel = AnisotropyLevel::None;

		float mipLodBias = 0.f;
		float minLod = 0.f;
		float maxLod = FLT_MAX;
	};

	inline static StaticSamplerDeclaration g_staticPointSampler =
	{
		.minFilter = TextureFilter::Nearest,
		.magFilter = TextureFilter::Nearest,
		.mipFilter = TextureFilter::Nearest,
		.wrapMode = TextureWrap::Repeat
	};

	inline static StaticSamplerDeclaration g_staticBilinearSampler =
	{
		.minFilter = TextureFilter::Linear,
		.magFilter = TextureFilter::Linear,
		.mipFilter = TextureFilter::Nearest,
		.wrapMode = TextureWrap::Repeat
	};

	inline static StaticSamplerDeclaration g_staticTrilinearSampler =
	{
		.minFilter = TextureFilter::Linear,
		.magFilter = TextureFilter::Linear,
		.mipFilter = TextureFilter::Linear,
		.wrapMode = TextureWrap::Repeat
	};

	inline static StaticSamplerDeclaration g_staticAnisotropicSampler =
	{
		.minFilter = TextureFilter::Linear,
		.magFilter = TextureFilter::Linear,
		.mipFilter = TextureFilter::Linear,
		.wrapMode = TextureWrap::Repeat,
		.anisotropyLevel = AnisotropyLevel::X16
	};
}
