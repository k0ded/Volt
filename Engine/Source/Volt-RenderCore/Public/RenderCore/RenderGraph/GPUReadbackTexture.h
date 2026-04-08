#pragma once

#include <RHIModule/Images/Image.h>

#include <CoreUtilities/Pointers/IntRef.h>

namespace Volt
{
	namespace RHI
	{
		class Image;
	}

	struct RGTextureDesc;

	class GPUReadbackTexture
	{
	public:
		GPUReadbackTexture(const RGTextureDesc& desc);

		VT_NODISCARD VT_INLINE IntRef<RHI::Image> GetImage() const { return m_image; }
		VT_NODISCARD VT_INLINE bool IsReady() const { return m_isReady.load(); }

	private:
		friend class RenderGraph;

		std::atomic_bool m_isReady = false;
		IntRef<RHI::Image> m_image;
	};
}
