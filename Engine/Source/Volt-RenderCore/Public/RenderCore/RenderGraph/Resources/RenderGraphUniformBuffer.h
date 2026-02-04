#pragma once

#include "RenderCore/Config.h"
#include "RenderCore/RenderGraph/Resources/RenderGraphResource.h"

#include <RHIModule/Buffers/UniformBuffer.h>

#include <RHIModule/Buffers/BufferView.h>

namespace Volt
{
	struct RGUniformBufferDesc : public RHI::UniformBufferDesc
	{
		template<typename T>
		static RGUniformBufferDesc Create(const std::string& name = "UniformBuffer")
		{
			RGUniformBufferDesc result;
			result.size = sizeof(T);
			result.debugName = name;

			return result;
		}
	};

	class RGRHIUniformBufferResource;

	class VTRC_API RGUniformBuffer : public RGResource
	{
	public:
		RGUniformBuffer(const RGUniformBufferDesc& desc);
		~RGUniformBuffer() override = default;
		RGResourceType GetResourceType() const override;

		bool HasProducer(RGResourceUAV* uav) const override;
		bool HasProducer() const override;
		void AddProducer(RenderGraphPass* pass, RGResourceUAV* uav) override;
		void AddProducer(RenderGraphPass* pass) override;

		VT_INLINE void AssignRHIResource(RGRHIUniformBufferResource* resource) { m_rhiResource = resource; }

		VT_NODISCARD VT_INLINE const RGUniformBufferDesc& GetDesc() const { return m_desc; }
		VT_NODISCARD VT_INLINE RGRHIUniformBufferResource* GetRHIResource() const { return m_rhiResource; }

	private:
		bool m_isProduced = false;
		RGUniformBufferDesc m_desc;

		RGRHIUniformBufferResource* m_rhiResource;
	};

	using RGUniformBufferRef = RGUniformBuffer*;

	struct RGUniformBufferSRVDesc
	{
		RGUniformBufferRef bufferResource = nullptr;

		size_t offset = 0;
		size_t size = std::numeric_limits<size_t>::max();
	};

	class VTRC_API RGUniformBufferSRV : public RGResourceSRV
	{
	public:
		RGUniformBufferSRV(const RGUniformBufferSRVDesc& desc);
		~RGUniformBufferSRV() override = default;

		RGResourceRef GetResource() const override { return m_desc.bufferResource; }

		VT_INLINE void AssignRHIView(RefPtr<RHI::BufferView> view) { m_rhiView = view; }
		VT_INLINE RefPtr<RHI::BufferView> GetRHIView() { return m_rhiView; }
		VT_INLINE const RGUniformBufferSRVDesc& GetDesc() { return m_desc; }

	private:
		RefPtr<RHI::BufferView> m_rhiView;
		RGUniformBufferSRVDesc m_desc;
	};

}
