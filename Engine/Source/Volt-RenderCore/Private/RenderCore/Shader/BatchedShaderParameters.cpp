#include "rcpch.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

#include <RHIModule/Shader/ShaderParameterMap.h>

namespace Volt
{
	void BatchedShaderParameters::AddBufferParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::BufferView> bufferView)
	{
		BatchedBufferShaderParameter* parameter = m_allocator.Allocate<BatchedBufferShaderParameter>(bindingName, resourceType, bufferView);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddTextureParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::ImageView> imageView)
	{
		BatchedTextureShaderParameter* parameter = m_allocator.Allocate<BatchedTextureShaderParameter>(bindingName, resourceType, imageView);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddSamplerParameter(const StringHash bindingName, const RHI::ShaderResourceType resourceType, RefPtr<RHI::SamplerState> sampler)
	{
		BatchedSamplerShaderParameter* parameter = m_allocator.Allocate<BatchedSamplerShaderParameter>(bindingName, resourceType, sampler);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::BindParametersToDescriptorTable(const Vector<RHI::ShaderParameterMap>& shaderParameterMaps, RefPtr<RHI::DescriptorTable> descriptorTable) const
	{
		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			for (const BatchedShaderParameter* parameter : m_parameters)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(parameter->bindingName);
				if (resourceBinding && resourceBinding->resourceType == parameter->resourceType)
				{
					switch (parameter->resourceType)
					{
						case RHI::ShaderResourceType::Texture:
						{
							const BatchedTextureShaderParameter* textureParameter = reinterpret_cast<const BatchedTextureShaderParameter*>(parameter);
							descriptorTable->SetImageView(textureParameter->imageView, resourceBinding->set, resourceBinding->binding);

							break;
						}

						case RHI::ShaderResourceType::StructuredBuffer:
						case RHI::ShaderResourceType::TexelBuffer:
						case RHI::ShaderResourceType::UniformBuffer:
						{
							const BatchedBufferShaderParameter* bufferParameter = reinterpret_cast<const BatchedBufferShaderParameter*>(parameter);
							descriptorTable->SetBufferView(bufferParameter->bufferView, resourceBinding->set, resourceBinding->binding);

							break;
						}

						case RHI::ShaderResourceType::Sampler:
						{
							const BatchedSamplerShaderParameter* samplerParameter = reinterpret_cast<const BatchedSamplerShaderParameter*>(parameter);
							descriptorTable->SetSamplerState(samplerParameter->sampler, resourceBinding->set, resourceBinding->binding);
						}
					}
				}
			}
		}
	}
}
