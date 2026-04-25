#include "rcpch.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

#include <RHIModule/Shader/ShaderParameterMap.h>
#include <RHIModule/Globals.h>
#include <RHIModule/RHIFeatures.h>

namespace Volt
{
	void BatchedShaderParameters::AddBufferParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::BufferView> bufferView)
	{
		BatchedBufferShaderBinding* parameter = m_allocator.Allocate<BatchedBufferShaderBinding>(bindingName, resourceType, bufferView);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddTextureParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::ImageView> imageView)
	{
		BatchedTextureShaderBinding* parameter = m_allocator.Allocate<BatchedTextureShaderBinding>(bindingName, resourceType, imageView);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddSamplerParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, IntRef<RHI::SamplerState> sampler)
	{
		BatchedSamplerShaderBinding* parameter = m_allocator.Allocate<BatchedSamplerShaderBinding>(bindingName, resourceType, sampler);
		m_bindings.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddShaderParameter(const StringHash parameterName, const void* data, const size_t size)
	{
		BatchedShaderParameter* parameter = m_allocator.Allocate<BatchedShaderParameter>(parameterName, data, size);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::PopulateShaderParameterUniformBuffers(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, Vector<RenderContext::PerStageShaderParameters, InlineAllocator<8>>& outShaderParameters)
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

		if (RHI::RHICanUseBindless())
		{
			PopulateShaderParameterUniformBuffersBindless(shaderParameterMaps, outShaderParameters);
		}
	}

	void BatchedShaderParameters::BindToShaderBindings(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings)
	{
		VT_PROFILE_FUNCTION();

		if (RHI::RHICanUseBindless())
		{
			BindToShaderBindingsBindlessInternal(shaderParameterMaps, shaderBindings);
		}
		else
		{
			BindToShaderBindingsInternal(shaderParameterMaps, shaderBindings);
		}
	}

	void BatchedShaderParameters::BindToShaderBindingsBindlessInternal(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings)
	{
		VT_PROFILE_FUNCTION();

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			if (!parameterMap.HasShaderBindings())
			{
				continue;
			}

			for (const BatchedShaderBinding* binding : m_bindings)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(binding->bindingName);

				// We only care about uniform buffers when bindless is enabled.
				if (resourceBinding &&
					resourceBinding->resourceType == binding->resourceType &&
					resourceBinding->resourceType == RHI::ShaderResourceType::UniformBuffer)
				{
					const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
					shaderBindings.SetUniformBuffer(parameterMap.GetShaderStage(), resourceBinding->binding, bufferParameter->bufferView);
				}
			}
		}
	}

	void BatchedShaderParameters::BindToShaderBindingsInternal(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, RHI::ShaderBindingMap& shaderBindings)
	{
		VT_PROFILE_FUNCTION();

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			if (!parameterMap.HasShaderBindings())
			{
				continue;
			}

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
							shaderBindings.SetTextureSRV(parameterMap.GetShaderStage(), resourceBinding->binding, textureParameter->imageView);

							break;
						}

						case RHI::ShaderResourceType::StructuredBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							shaderBindings.SetStructuredBufferSRV(parameterMap.GetShaderStage(), resourceBinding->binding, bufferParameter->bufferView);

							break;
						}

						case RHI::ShaderResourceType::TexelBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							shaderBindings.SetTexelBufferSRV(parameterMap.GetShaderStage(), resourceBinding->binding, bufferParameter->bufferView);

							break;
						}

						case RHI::ShaderResourceType::UniformBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							shaderBindings.SetUniformBuffer(parameterMap.GetShaderStage(), resourceBinding->binding, bufferParameter->bufferView);

							break;
						}

						case RHI::ShaderResourceType::Sampler:
						{
							const BatchedSamplerShaderBinding* samplerParameter = reinterpret_cast<const BatchedSamplerShaderBinding*>(binding);
							shaderBindings.SetSampler(parameterMap.GetShaderStage(), resourceBinding->binding, samplerParameter->sampler);
						}
					}
				}
			}
		}
	}

	void BatchedShaderParameters::PopulateShaderParameterUniformBuffersBindless(ArrayView<RHI::ShaderParameterMap> shaderParameterMaps, Vector<RenderContext::PerStageShaderParameters, InlineAllocator<8>>& outShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		auto SetBindlessIndex = [&](RHI::BindlessIndex bindlessIndex, const RHI::ShaderResourceBinding* resourceBinding)
		{
			VT_ENSURE(bindlessIndex.IsValid());

			for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
			{
				const RHI::ShaderUniform* shaderParameter = parameterMap.GetParameterFromName(resourceBinding->bindlessHash);
				if (shaderParameter)
				{
					for (const auto& perStageParameters : outShaderParameters)
					{
						if (perStageParameters.shaderStage == parameterMap.GetShaderStage())
						{
							uint32_t tempIndex = bindlessIndex.Get();
							memcpy(perStageParameters.mappedPtr + shaderParameter->offset, &tempIndex, sizeof(tempIndex));
							break;
						}
					}
				}
			}
		};

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			if (!parameterMap.HasShaderBindings())
			{
				continue;
			}

			for (const BatchedShaderBinding* binding : m_bindings)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(binding->bindingName);
				if (resourceBinding && resourceBinding->resourceType == binding->resourceType)
				{
					if (binding->resourceType == RHI::ShaderResourceType::UniformBuffer)
					{
						continue;
					}

					RHI::BindlessIndex bindlessIndex{};

					switch (binding->resourceType)
					{
						case RHI::ShaderResourceType::Texture:
						{
							const BatchedTextureShaderBinding* textureParameter = reinterpret_cast<const BatchedTextureShaderBinding*>(binding);
							bindlessIndex = textureParameter->imageView->GetSRVBindlessIndex();
							break;
						}

						case RHI::ShaderResourceType::StructuredBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							bindlessIndex = bufferParameter->bufferView->GetSRVBindlessIndex();
							break;
						}

						case RHI::ShaderResourceType::TexelBuffer:
						{
							const BatchedBufferShaderBinding* bufferParameter = reinterpret_cast<const BatchedBufferShaderBinding*>(binding);
							bindlessIndex = bufferParameter->bufferView->GetSRVBindlessIndex();
							break;
						}

						case RHI::ShaderResourceType::Sampler:
						{
							const BatchedSamplerShaderBinding* samplerParameter = reinterpret_cast<const BatchedSamplerShaderBinding*>(binding);
							bindlessIndex = samplerParameter->sampler->GetBindlessIndex();
						}
					}

					SetBindlessIndex(bindlessIndex, resourceBinding);
				}
			}
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
