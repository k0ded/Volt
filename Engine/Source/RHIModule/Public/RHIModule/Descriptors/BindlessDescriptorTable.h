#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Descriptors/ResourceHandle.h"

#include <CoreUtilities/Pointers/RawPtr.h>

namespace Volt::RHI
{
	class StorageBuffer;
	class ImageView;
	class SamplerState;
	class CommandBuffer;
	class UniformBuffer;
	class AccelerationStructure;

	class VTRHI_API BindlessDescriptorTable : public RHIInterface
	{
	public:
		~BindlessDescriptorTable() override = default;

		virtual ResourceHandle RegisterBuffer(RawPtr<StorageBuffer> storageBuffer) = 0;
		virtual ResourceHandle RegisterImageView(RawPtr<ImageView> imageView) = 0;
		virtual ResourceHandle RegisterSamplerState(RawPtr<SamplerState> samplerState) = 0;

		virtual void UnregisterResource(ResourceHandle handle) = 0;
		virtual void MarkResourceAsDirty(ResourceHandle handle) = 0;

		virtual void UnregisterSamplerState(ResourceHandle handle) = 0;
		virtual void MarkSamplerStateAsDirty(ResourceHandle handle) = 0;

		virtual void Update() = 0;
		virtual void PrepareForRender() = 0;
		virtual bool IsResourceValid(ResourceHandle handle) const = 0;

		virtual void Bind(CommandBuffer& commandBuffer, RawPtr<UniformBuffer> constantsBuffer, const uint32_t offsetIndex, const uint32_t stride, RawPtr<AccelerationStructure> accelerationStructure) = 0;

		static RefPtr<BindlessDescriptorTable> Create(const uint64_t framesInFlight);

	protected:
		BindlessDescriptorTable() = default;
	};
}
