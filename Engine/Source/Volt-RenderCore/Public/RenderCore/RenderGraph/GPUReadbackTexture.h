#pragma once

#include <CoreUtilities/Pointers/RefPtr.h>

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

		VT_NODISCARD VT_INLINE RefPtr<RHI::Image> GetImage() const { return m_image; }
		VT_NODISCARD VT_INLINE bool IsReady() const { return m_isReady.load(); }

	private:
		friend class RenderGraph2;

		std::atomic_bool m_isReady = false;
		RefPtr<RHI::Image> m_image;
	};
}
