#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph2/Resources/RenderGraphResource.h"

namespace Volt
{
	struct RGUniformBufferDesc
	{
		uint32_t count;
		uint64_t elementSize;
		std::string name;
	};

	class VTRC_API RGUniformBuffer : public RGResource
	{
	public:
		RGUniformBuffer(const RGUniformBufferDesc& desc);
		~RGUniformBuffer() override = default;
		RGResourceType GetResourceType() const override;

		VT_NODISCARD VT_INLINE const RGUniformBufferDesc& GetDesc() const { return m_desc; }

	private:
		RGUniformBufferDesc m_desc;
	};
}
