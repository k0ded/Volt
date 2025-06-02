#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include <CoreUtilities/Pointers/RawPtr.h>

namespace Volt::RHI
{
	class Shader;
	class ImageView;
	class BufferView;
	class BufferViewSet;
	class ComputePipeline;
	class RenderPipeline;

	class SamplerState;

	class CommandBuffer;

	struct DescriptorTableCreateInfo
	{
		RefPtr<Shader> shader;
		RefPtr<ComputePipeline> computePipeline;
		RefPtr<RenderPipeline> renderPipeline;
	};

	class VTRHI_API DescriptorTable : public RHIInterface
	{
	public:
		virtual void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) = 0;
		virtual void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) = 0;
		virtual void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, uint32_t arrayIndex = 0) = 0;

		virtual void SetImageView(std::string_view name, RawPtr<ImageView> view, uint32_t arrayIndex) = 0;
		virtual void SetBufferView(std::string_view name, RawPtr<BufferView> view, uint32_t arrayIndex) = 0;
		virtual void SetSamplerState(std::string_view name, RawPtr<SamplerState> samplerState, uint32_t arrayIndex) = 0;

		virtual void PrepareForRender() = 0;

		static RefPtr<DescriptorTable> Create(const DescriptorTableCreateInfo& specification);
		static RefPtr<DescriptorTable> Create2(const DescriptorTableCreateInfo& specification);

	protected:
		virtual void Bind(CommandBuffer& commandBuffer) = 0;

		DescriptorTable() = default;
	};
}
