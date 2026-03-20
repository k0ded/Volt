#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

namespace Volt::RHI
{
	struct SamplerStateDesc
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

	class VTRHI_API SamplerState : public ArenaRHIInterface
	{
	public:
		static IntRef<SamplerState> Create(const SamplerStateDesc& createInfo);

	protected:
		SamplerState() = default;
	};
}
