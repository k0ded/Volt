#pragma once

#include "RenderCore/Config.h"

#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

namespace Volt
{
	struct RGUniformBufferDesc
	{
		uint32_t count;
		uint64_t elementSize;
		std::string name;

		template<typename T>
		static RGUniformBufferDesc Create(const std::string& name = "UniformBuffer")
		{
			RGUniformBufferDesc result;
			result.count = 1;
			result.elementSize = sizeof(T);
			result.name = name;

			return result;
		}
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
