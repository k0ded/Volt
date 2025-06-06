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

		static RefPtr<RHI::SamplerState> GetPointSampler();
		static RefPtr<RHI::SamplerState> GetBilinearSampler();
		static RefPtr<RHI::SamplerState> GetTrilinearSampler();

		template<RHI::TextureFilter min, RHI::TextureFilter mag, RHI::TextureFilter mip, 
				RHI::TextureWrap wrapMode = RHI::TextureWrap::Repeat, RHI::AnisotropyLevel aniso = RHI::AnisotropyLevel::None, 
				RHI::CompareOperator compareOperator = RHI::CompareOperator::None>
		static RefPtr<RHI::SamplerState> GetSampler()
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
	
		RefPtr<RHI::SamplerState> GetSamplerInternal(const RHI::SamplerStateDesc& samplerDesc);

		std::mutex m_cacheMutex;
		vt::map<size_t, RefPtr<RHI::SamplerState>> m_cache;
	};
}
