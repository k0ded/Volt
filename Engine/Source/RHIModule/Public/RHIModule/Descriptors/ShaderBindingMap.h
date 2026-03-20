#pragma once

#include "RHIModule/Core/Core.h"

#include "RHIModule/Buffers/BufferView.h"
#include "RHIModule/Images/ImageView.h"
#include "RHIModule/Images/SamplerState.h"
#include "RHIModule/RayTracing/AccelerationStructure.h"
#include "RHIModule/Descriptors/ResourceTable.h"
#include "RHIModule/Shader/ShaderCommon.h"

#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/Containers/BitArray.h>
#include <CoreUtilities/Containers/ArrayView.h>

namespace Volt::RHI
{
	class RenderPipeline;
	class ComputePipeline;

	class VTRHI_API ShaderBindingMap
	{
	public:
		inline static constexpr uint32_t NumMaxBindings = 32u;
		struct ResourceBinding
		{
			Variant<
				IntRef<RHI::BufferView>,
				IntRef<RHI::ImageView>,
				IntRef<RHI::SamplerState>,
				IntRef<RHI::AccelerationStructure>
			> resource;

			uint64_t uniformBufferSize = 0;
			uint64_t uniformBufferOffset = 0;

			uint32_t bindingIndex;
			ShaderRegisterType registerType;
			ShaderResourceType resourceType;
		};

		struct PerShaderStageResourceBindings
		{
			ShaderStage shaderStage;
			InlineVector<ResourceBinding, NumMaxBindings> resourceBindings;
			Array<BitArray<NumMaxBindings>, static_cast<size_t>(ShaderRegisterType::Max)> resourceIsSet;
		};

		ShaderBindingMap() = default;

		void SetUniformBuffer(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView);
		void SetUniformBufferWithSizeAndOffset(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView, uint64_t size, uint64_t offset);
		void SetSampler(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::SamplerState> samplerState);
		void SetStructuredBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView);
		void SetStructuredBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView);
		void SetTexelBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView);
		void SetTexelBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::BufferView> bufferView);
		void SetTextureSRV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::ImageView> imageView);
		void SetTextureUAV(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::ImageView> imageView);
		void SetAccelerationStructure(ShaderStage shaderStage, uint32_t bindingIndex, IntRef<RHI::AccelerationStructure> accelerationStructure);
		void SetResourceTable(IntRef<ResourceTable> resourceTable);

		VT_NODISCARD VT_INLINE const ArrayView<PerShaderStageResourceBindings> GetBindings() const { return m_resourceBindings; }
		VT_NODISCARD VT_INLINE IntRef<ResourceTable> GetResourceTable() const { return m_resourceTable; }
		VT_NODISCARD VT_INLINE bool HasResourceTable() const { return m_resourceTable != nullptr; }

		static ShaderBindingMap InitializeFromPipeline(RawPtr<RenderPipeline> renderPipeline);
		static ShaderBindingMap InitializeFromPipeline(RawPtr<ComputePipeline> computePipeline);

	private:
		ShaderBindingMap(const InlineVector<ShaderStage, GetNumBindableShaderStages()>& shaderStages);

		bool IsResourceSet(ShaderStage shaderStage, ShaderRegisterType registerType, uint32_t bindingIndex) const;
		void MarkResourceAsSet(ShaderStage shaderStage, ShaderRegisterType registerType, uint32_t bindingIndex);

		InlineVector<ResourceBinding, NumMaxBindings>& GetResourceBindingsForShaderStage(ShaderStage shaderStage);

		BitArray<GetNumBindableShaderStages(), uint32_t> m_activeShaderStagesBitArray;
		GlobalMemoryStackVector<PerShaderStageResourceBindings> m_resourceBindings;

		IntRef<ResourceTable> m_resourceTable;
	};
}
