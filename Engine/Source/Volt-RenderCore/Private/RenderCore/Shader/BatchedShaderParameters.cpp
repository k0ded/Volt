#include "rcpch.h"
#include "RenderCore/Shader/BatchedShaderParameters.h"

namespace Volt
{
	void BatchedShaderParameters::AddBufferParameter(const RHI::ShaderResourceBinding* resourceBinding, RefPtr<RHI::BufferView> bufferView)
	{
		BatchedBufferShaderParameter* parameter = m_allocator.Allocate<BatchedBufferShaderParameter>(resourceBinding, bufferView);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::AddTextureParameter(const RHI::ShaderResourceBinding* resourceBinding, RefPtr<RHI::ImageView> imageView)
	{
		BatchedTextureShaderParameter* parameter = m_allocator.Allocate<BatchedTextureShaderParameter>(resourceBinding, imageView);
		m_parameters.emplace_back(parameter);
	}

	void BatchedShaderParameters::BindParametersToDescriptorTable(RefPtr<RHI::DescriptorTable> descriptorTable) const
	{
		for (const BatchedShaderParameter* parameter : m_parameters)
		{
			switch (parameter->resourceBinding->resourceType)
			{
				case RHI::ShaderResourceType::Texture:
				{
					const BatchedTextureShaderParameter* textureParameter = reinterpret_cast<const BatchedTextureShaderParameter*>(parameter);
					descriptorTable->SetImageView(textureParameter->imageView, textureParameter->resourceBinding->set, textureParameter->resourceBinding->binding);

					break;
				}

				case RHI::ShaderResourceType::StructuredBuffer:
				case RHI::ShaderResourceType::TexelBuffer:
				case RHI::ShaderResourceType::UniformBuffer:
				{
					const BatchedBufferShaderParameter* bufferParameter = reinterpret_cast<const BatchedBufferShaderParameter*>(parameter);
					descriptorTable->SetBufferView(bufferParameter->bufferView, bufferParameter->resourceBinding->set, bufferParameter->resourceBinding->binding);

					break;
				}
			}
		}
	}
}
