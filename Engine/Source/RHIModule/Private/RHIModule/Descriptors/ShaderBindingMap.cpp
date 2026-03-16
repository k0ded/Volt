#include "rhipch.h"

#include "RHIModule/Descriptors/ShaderBindingMap.h"
#include "RHIModule/Shader/Shader.h"
#include "RHIModule/Pipelines/RenderPipeline.h"

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt::RHI
{
	void ShaderBindingMap::SetUniformBuffer(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::CBV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::CBV;
			resourceBinding.resourceType = ShaderResourceType::UniformBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::CBV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetSampler(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::SamplerState> samplerState)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::Sampler, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::Sampler;
			resourceBinding.resourceType = ShaderResourceType::Sampler;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = samplerState;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::Sampler, bindingIndex);
		}
	}

	void ShaderBindingMap::SetStructuredBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::UAV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::UAV;
			resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::UAV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetStructuredBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::SRV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);
		
			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::SRV;
			resourceBinding.resourceType = ShaderResourceType::StructuredBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::SRV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetTexelBufferUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::UAV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::UAV;
			resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::UAV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetTexelBufferSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::SRV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::SRV;
			resourceBinding.resourceType = ShaderResourceType::TexelBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::SRV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetTextureSRV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::SRV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::SRV;
			resourceBinding.resourceType = ShaderResourceType::Texture;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = imageView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::SRV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetTextureUAV(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::ImageView> imageView)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::UAV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::UAV;
			resourceBinding.resourceType = ShaderResourceType::Texture;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = imageView;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::UAV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetAccelerationStructure(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::AccelerationStructure> accelerationStructure)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::SRV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::SRV;
			resourceBinding.resourceType = ShaderResourceType::AccelerationStructure;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = accelerationStructure;
		
			MarkResourceAsSet(shaderStage, ShaderRegisterType::SRV, bindingIndex);
		}
	}

	void ShaderBindingMap::SetUniformBufferWithSizeAndOffset(ShaderStage shaderStage, uint32_t bindingIndex, RefPtr<RHI::BufferView> bufferView, uint64_t size, uint64_t offset)
	{
		if (!IsResourceSet(shaderStage, ShaderRegisterType::CBV, bindingIndex))
		{
			auto& resourceBindings = GetResourceBindingsForShaderStage(shaderStage);

			auto& resourceBinding = resourceBindings.emplace_back();
			resourceBinding.registerType = ShaderRegisterType::CBV;
			resourceBinding.resourceType = ShaderResourceType::UniformBuffer;
			resourceBinding.bindingIndex = bindingIndex;
			resourceBinding.resource = bufferView;
			resourceBinding.uniformBufferSize = size;
			resourceBinding.uniformBufferOffset = offset;

			MarkResourceAsSet(shaderStage, ShaderRegisterType::CBV, bindingIndex);
		}
	}

	ShaderBindingMap ShaderBindingMap::InitializeFromPipeline(RawPtr<RenderPipeline> renderPipeline)
	{
		VT_PROFILE_FUNCTION();

		InlineVector<ShaderStage, GetNumBindableShaderStages()> shaderStages;
		for (const RefPtr<Shader>& shader : renderPipeline->GetShaders())
		{
			shaderStages.emplace_back(shader->GetShaderStage());
		}

		return { shaderStages };
	}

	ShaderBindingMap ShaderBindingMap::InitializeFromPipeline(RawPtr<ComputePipeline> computePipeline)
	{
		return ShaderBindingMap({ ShaderStage::Compute });
	}

	ShaderBindingMap::ShaderBindingMap(const InlineVector<ShaderStage, GetNumBindableShaderStages()>& shaderStages)
	{
		for (auto shaderStage : shaderStages)
		{
			m_activeShaderStagesBitArray.SetBit(GetShaderStageIndex(shaderStage), true);
		}

		m_resourceBindings.resize(shaderStages.size());

		for (auto shaderStage : shaderStages)
		{
			const uint32_t rank = m_activeShaderStagesBitArray.Rank(GetShaderStageIndex(shaderStage));
			m_resourceBindings[rank].shaderStage = shaderStage;
		}
	}

	bool ShaderBindingMap::IsResourceSet(ShaderStage shaderStage, ShaderRegisterType registerType, uint32_t bindingIndex) const
	{
		const uint32_t bindingsIndex = m_activeShaderStagesBitArray.Rank(GetShaderStageIndex(shaderStage));
		return m_resourceBindings[bindingsIndex].resourceIsSet[static_cast<size_t>(registerType)].IsBitSet(bindingIndex);
	}

	void ShaderBindingMap::MarkResourceAsSet(ShaderStage shaderStage, ShaderRegisterType registerType, uint32_t bindingIndex)
	{
		const uint32_t bindingsIndex = m_activeShaderStagesBitArray.Rank(GetShaderStageIndex(shaderStage));
		m_resourceBindings[bindingsIndex].resourceIsSet[static_cast<size_t>(registerType)].SetBit(bindingIndex, true);
	}

	InlineVector<ShaderBindingMap::ResourceBinding, ShaderBindingMap::NumMaxBindings>& ShaderBindingMap::GetResourceBindingsForShaderStage(ShaderStage shaderStage)
	{
		const uint32_t bindingsIndex = m_activeShaderStagesBitArray.Rank(GetShaderStageIndex(shaderStage));
		return m_resourceBindings[bindingsIndex].resourceBindings;
	}

	void ShaderBindingMap::SetResourceTable(RefPtr<ResourceTable> resourceTable)
	{
		m_resourceTable = resourceTable;
	}
}
