#pragma once

#include "RenderCore/Config.h"

#include <RHIModule/Images/SamplerState.h>

namespace Volt
{
	class VTRC_API SamplerStateCache
	{
	public:
		SamplerStateCache();
		~SamplerStateCache();

		static IntRef<RHI::SamplerState> GetPointSampler();
		static IntRef<RHI::SamplerState> GetBilinearSampler();
		static IntRef<RHI::SamplerState> GetTrilinearSampler();
		static IntRef<RHI::SamplerState> GetAnisotropicSampler();

		template<RHI::TextureFilter min, RHI::TextureFilter mag, RHI::TextureFilter mip, 
				RHI::TextureWrap wrapMode = RHI::TextureWrap::Repeat, RHI::AnisotropyLevel aniso = RHI::AnisotropyLevel::None, 
				RHI::CompareOperator compareOperator = RHI::CompareOperator::None>
		static IntRef<RHI::SamplerState> GetSampler()
		{
			RHI::SamplerStateDesc desc{};
			desc.minFilter = min;
			desc.magFilter = mag;
			desc.mipFilter = mip;
			desc.wrapMode = wrapMode;
			desc.anisotropyLevel = aniso;
			desc.compareOperator = compareOperator;

			return s_instance->GetSamplerInternal(desc);
		}

	private:
		inline static SamplerStateCache* s_instance = nullptr;
	
		IntRef<RHI::SamplerState> GetSamplerInternal(const RHI::SamplerStateDesc& samplerDesc);

		std::mutex m_cacheMutex;
		Map<size_t, IntRef<RHI::SamplerState>> m_cache;
	};
}
