#include "rcpch.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

#include <RHIModule/Shader/ShaderParameterMap.h>
#include <RHIModule/Globals.h>

namespace Volt
{
	void BatchedShaderParameters::AddBufferParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::BufferView> bufferView)
	{
		BatchedBufferShaderBinding* parameter = m_allocator.Allocate<BatchedBufferShaderBinding>(bindingName, resourceType, bufferView);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddTextureParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::ImageView> imageView)
	{
		BatchedTextureShaderBinding* parameter = m_allocator.Allocate<BatchedTextureShaderBinding>(bindingName, resourceType, imageView);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddSamplerParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::SamplerState> sampler)
	{
		BatchedSamplerShaderBinding* parameter = m_allocator.Allocate<BatchedSamplerShaderBinding>(bindingName, resourceType, sampler);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddShaderParameter(const StringHash parameterName, const void* data, const size_t size)
	{
		BatchedShaderParameter* parameter = m_allocator.Allocate<BatchedShaderParameter>(parameterName, data, size);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::PopulateShaderParameterUniformBuffers(const Vector<RHI::ShaderParameterMap>& shaderParameterMaps, Vector<RenderContext::PerStageShaderParameters, InlineAllocator<8>>& outShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			for (const BatchedShaderParameter* parameter : m_parameters)
			{
				const RHI::ShaderUniform* shaderParameter = parameterMap.GetParameterFromName(parameter->parameterName);
				if (shaderParameter)
				{
					for (const auto& perStageParameters : outShaderParameters)
					{
						if (perStageParameters.shaderStage == parameterMap.GetShaderStage())
						{
							memcpy(perStageParameters.mappedPtr + shaderParameter->offset, parameter->data, parameter->size);
							break;
						}
					}
				}
			}
		}
	}

	void BatchedShaderParameters::BindShaderBindingsToDescriptorTable(const Vector<RHI::ShaderParameterMap>& shaderParameterMaps, RefPtr<RHI::DescriptorTable> descriptorTable, const InlineVector<RenderContext::PerStageShaderParameters, 8>& shaderParameterUniformBuffers) const
	{
		VT_PROFILE_FUNCTION();

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			for (const BatchedShaderBinding* binding : m_bindings)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(binding->bindingName);
				if (resourceBinding && resourceBinding->resourceType == binding->resourceType)
				{
					switch (binding->resourceType)
					{
						case RHI::ShaderResourceType::Texture:
						{
							const BatchedTextureShaderBinding* textureParameter = reinterpret_cast<const BatchedTextureShaderBinding*>(binding);
							descriptorTable->SetImageView(textureParameter->imageView, resourceBinding->set, resourceBinding->binding);

							break;
						}

						case RHI::ShaderResourceType::StructuredBuffer:
						case RHI::ShaderResourceType::TexelBuffer:
						case RHI::ShaderResourceType::UniformBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							descriptorTable->SetBufferView(bufferParameter->bufferView, resourceBinding->set, resourceBinding->binding);

							break;
						}

						case RHI::ShaderResourceType::Sampler:
						{
							const BatchedSamplerShaderBinding* samplerParameter = reinterpret_cast<const BatchedSamplerShaderBinding*>(binding);
							descriptorTable->SetSamplerState(samplerParameter->sampler, resourceBinding->set, resourceBinding->binding);
						}
					}
				}
			}
		}

		for (const RenderContext::PerStageShaderParameters& perStageParameters : shaderParameterUniformBuffers)
		{
			descriptorTable->SetBufferView(perStageParameters.uniformBufferSRV->GetRHIView(), RHI::GetDescriptorSetIndexFromShaderStage(perStageParameters.shaderStage), RHI::Globals::SHADER_GLOBALS_BINDING);
		}
	}

	BatchedShaderParameterAllocator::BatchedShaderParameterAllocator()
	{
		m_allocator.Reserve(MaxBatchedShaderParameterSize);
	}

	BatchedShaderParameterAllocator::~BatchedShaderParameterAllocator()
	{
		for (auto& destructor : m_destructors)
		{
			destructor.Destroy();
		}
	}
}
