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

		bool HasProducer(RGResourceUAV* uav) const override;
		bool HasProducer() const override;
		void AddProducer(Handle<RenderGraphPass> pass, RGResourceUAV* uav) override;
		void AddProducer(Handle<RenderGraphPass> pass) override;

		VT_NODISCARD VT_INLINE const RGUniformBufferDesc& GetDesc() const { return m_desc; }

	private:
		bool m_isProduced = false;
		RGUniformBufferDesc m_desc;
	};

	using RGUniformBufferRef = RGUniformBuffer*;

	class VTRC_API RGUniformBufferSRV : public RGResourceSRV
	{
	public:
		RGUniformBufferSRV(RGUniformBufferRef uniformBuffer);
		~RGUniformBufferSRV() override = default;

		RGResourceRef GetResource() const override { return m_resource; }

	private:
		RGUniformBufferRef m_resource;
	};

}
