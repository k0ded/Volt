#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Buffers/BufferView.h"

#include <CoreUtilities/Pointers/RawPtr.h>

namespace Volt::RHI
{
	class Shader;
	class ImageView;
	class BufferViewSet;
	class ComputePipeline;
	class RenderPipeline;

	class SamplerState;
	class AccelerationStructure;
	class RayTracingResourceTable;

	class CommandBuffer;

	struct DescriptorTableCreateInfo
	{
		RefPtr<ComputePipeline> computePipeline;
		RefPtr<RenderPipeline> renderPipeline;
	};

	class VTRHI_API DescriptorTable : public RHIInterface
	{
	public:
		virtual void SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding) = 0;
		virtual void SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding) = 0;
		virtual void SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding) = 0;
		virtual void SetAccelerationStructure(RawPtr<AccelerationStructure> accelerationStructure, uint32_t set, uint32_t binding) = 0;
		virtual void SetRayTracingResourceTable(RefPtr<RayTracingResourceTable> rayTracingResourceTable) = 0;

		virtual size_t GetHash() const = 0;

		virtual void PrepareForRender() = 0;

		static RefPtr<DescriptorTable> Create(const DescriptorTableCreateInfo& specification);

	protected:
		DescriptorTable() = default;
	};
}
