#pragma once

#include "RHIModule/Core/Core.h"

#include "RHIModule/Buffers/BufferView.h"
#include "RHIModule/Images/ImageView.h"
#include "RHIModule/Images/SamplerState.h"
#include "RHIModule/RayTracing/AccelerationStructure.h"
#include "RHIModule/Shader/ShaderCommon.h"

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/VectorVariants.h>

namespace Volt::RHI
{
	class VTRHI_API ShaderBindingMap
	{
	public:
		inline static constexpr uint32_t NumMaxBindings = 32u;
		struct ResourceBinding
		{
			RefPtr<RHI::BufferView> bufferView;
			RefPtr<RHI::ImageView> imageView;
			RefPtr<RHI::SamplerState> samplerState;
			RefPtr<RHI::AccelerationStructure> accelerationStructure;
			ShaderRegisterType registerType;
			ShaderResourceType resourceType;
			uint32_t bindingIndex;
		};

		using ResourceBindingsMap = Map<ShaderStage, InlineVector<ResourceBinding, NumMaxBindings>>;

		void SetUniformBuffer(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView);
		void SetSampler(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::SamplerState> samplerState);
		void SetStructuredBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView);
		void SetStructuredBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView);
		void SetTexelBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView);
		void SetTexelBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView);
		void SetTextureSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView);
		void SetTextureUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView);
		void SetAccelerationStructure(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::AccelerationStructure> accelerationStructure);

		VT_NODISCARD VT_INLINE const ResourceBindingsMap& GetBindings() const { return m_resourceBindings; }

	private:
		ResourceBindingsMap m_resourceBindings;
	};
}
