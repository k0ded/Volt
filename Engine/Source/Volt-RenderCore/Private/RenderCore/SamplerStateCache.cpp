#include "rcpch.h"
#include "RenderCore/SamplerStateCache.h"

#include <CoreUtilities/Math/Hash.h>

namespace Volt
{
	namespace Utility
	{
		inline static const size_t GetHashFromSamplerDesc(const RHI::SamplerStateDesc& info)
		{
			size_t hash = std::hash<uint32_t>()(static_cast<uint32_t>(info.minFilter));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.magFilter)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.mipFilter)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.wrapMode)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.compareOperator)));
			hash = Math::HashCombine(hash, std::hash<uint32_t>()(static_cast<uint32_t>(info.anisotropyLevel)));
			hash = Math::HashCombine(hash, std::hash<float>()(info.mipLodBias));
			hash = Math::HashCombine(hash, std::hash<float>()(info.minLod));
			hash = Math::HashCombine(hash, std::hash<float>()(info.maxLod));

			return hash;
		}
	}

	SamplerStateCache::SamplerStateCache()
	{
		VT_ENSURE(!s_instance);
		s_instance = this;
	}

	SamplerStateCache::~SamplerStateCache()
	{
		s_instance = nullptr;
	}

	IntRef<RHI::SamplerState> SamplerStateCache::GetPointSampler()
	{
		return GetSampler<RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest, RHI::TextureFilter::Nearest>();
	}

	IntRef<RHI::SamplerState> SamplerStateCache::GetBilinearSampler()
	{
		return GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Nearest>();
	}

	IntRef<RHI::SamplerState> SamplerStateCache::GetTrilinearSampler()
	{
		return GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear>();
	}

	IntRef<RHI::SamplerState> SamplerStateCache::GetAnisotropicSampler()
	{
		return GetSampler<RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, RHI::TextureFilter::Linear, 
						RHI::TextureWrap::Repeat, RHI::AnisotropyLevel::X16>();
	}

	IntRef<RHI::SamplerState> SamplerStateCache::GetSamplerInternal(const RHI::SamplerStateDesc& samplerDesc)
	{
		const size_t samplerHash = Utility::GetHashFromSamplerDesc(samplerDesc);

		{
			std::scoped_lock lock{ m_cacheMutex };
			if (m_cache.contains(samplerHash))
			{
				return m_cache.at(samplerHash);
			}
		}

		IntRef<RHI::SamplerState> samplerState = RHI::SamplerState::Create(samplerDesc);
		{
			std::scoped_lock lock{ m_cacheMutex };
			m_cache[samplerHash] = samplerState;
		}

		return samplerState;
	}
}
