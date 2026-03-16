#pragma once

namespace Volt
{
	template<typename T>
	T* RenderContext::MapBuffer(RGBufferRef buffer)
	{
		return reinterpret_cast<T*>(MapInternal(buffer));
	}

	template<typename T>
	T* RenderContext::MapBuffer(RGUniformBufferRef buffer)
	{
		return reinterpret_cast<T*>(MapInternal(buffer));
	}

	template<typename ShaderType>
	void RenderContext::SetParameters(RefPtr<RHI::Shader> shader, const typename ShaderType::Parameters* parameters)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(m_currentRenderPipeline || m_currentComputePipeline);

		using ShaderParametersType = typename ShaderType::Parameters;

		VerifyShaderParameters(shader, parameters);

		const ShaderParameterMetadataDescription* parameterStructMetadataDesc = ShaderParametersType::GetShaderParameterMetadata();
		const auto& shaderParameterMap = shader->GetParameterMap();

		RenderGraphParameterStruct renderGraphParameterStruct(parameters, parameterStructMetadataDesc);

		renderGraphParameterStruct.EnumerateParameters([this, &shaderParameterMap](RenderGraphParameterDesc parameterDesc)
		{
			switch (parameterDesc.GetType())
			{
				case ShaderParameterType::BufferSRV: SetBufferSRVParameter(parameterDesc.GetAs<RGBufferSRVRef>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::BufferUAV: SetBufferUAVParameter(parameterDesc.GetAs<RGBufferUAVRef>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::TextureSRV: SetTextureSRVParameter(parameterDesc.GetAs<RGTextureSRVRef>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::TextureUAV: SetTextureUAVParameter(parameterDesc.GetAs<RGTextureUAVRef>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::UniformBuffer: SetUniformBufferParameter(parameterDesc.GetAs<RGUniformBufferRef>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::Sampler: SetSamplerParameter(parameterDesc.GetAs<RefPtr<RHI::SamplerState>>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::AccelerationStructure: SetAccelerationStructureParameter(parameterDesc.GetAs<RefPtr<RHI::AccelerationStructure>>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::ResourceTable: SetResourceTableParameter(parameterDesc.GetAs<RefPtr<RHI::ResourceTable>>(), parameterDesc, shaderParameterMap); break;
				case ShaderParameterType::Parameter: SetShaderParameter(parameterDesc.GetData(), parameterDesc, shaderParameterMap); break;
			}
		});
	}

	template<typename ParameterStruct>
	void RenderContext::CollectParameters(const ParameterStruct* parameters, BatchedShaderParameters& batchedShaderParameters)
	{
		VT_PROFILE_FUNCTION();

		const ShaderParameterMetadataDescription* parameterStructMetadataDesc = ParameterStruct::GetShaderParameterMetadata();
		RenderGraphParameterStruct renderGraphParameterStruct(parameters, parameterStructMetadataDesc);

		renderGraphParameterStruct.EnumerateParameters([this, &batchedShaderParameters](RenderGraphParameterDesc parameterDesc)
		{
			switch (parameterDesc.GetType())
			{
				case ShaderParameterType::BufferSRV: CollectBufferSRVParameter(parameterDesc.GetAs<RGBufferSRVRef>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::BufferUAV: CollectBufferUAVParameter(parameterDesc.GetAs<RGBufferUAVRef>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::TextureSRV: CollectTextureSRVParameter(parameterDesc.GetAs<RGTextureSRVRef>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::TextureUAV: CollectTextureUAVParameter(parameterDesc.GetAs<RGTextureUAVRef>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::Sampler: CollectSamplerParameter(parameterDesc.GetAs<RefPtr<RHI::SamplerState>>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::UniformBuffer: CollectUniformBufferParameter(parameterDesc.GetAs<RGUniformBufferRef>(), parameterDesc, batchedShaderParameters); break;
				case ShaderParameterType::Parameter: CollectShaderParameter(parameterDesc.GetData(), parameterDesc, batchedShaderParameters); break;
			}
		});
	}

	template<typename ParameterStruct>
	void RenderContext::VerifyShaderParameters(RefPtr<RHI::Shader> shader, const ParameterStruct* parameters)
	{
		VT_PROFILE_FUNCTION();

		const ShaderParameterMetadataDescription* parameterStructMetadataDesc = ParameterStruct::GetShaderParameterMetadata();

		const RHI::ShaderParameterMap& shaderParameterMap = shader->GetParameterMap();
		const RHI::ShaderParameterMap::ResourceBindings& resourceBindings = shaderParameterMap.GetResourceBindings();

		struct Binding
		{
			StringHash hash;
			std::string_view name;
			bool value;
		};

		GlobalMemoryStackMark memStackMark;
		GlobalMemoryStackVector<Binding> foundResourceBindings;
		foundResourceBindings.reserve(resourceBindings.size());

		STRING_HASH_CONSTEXPR StringHash GlobalsStringHash = StringHash::Construct("$Globals");
		STRING_HASH_CONSTEXPR StringHash RayTracingBufferTableHash = StringHash::Construct("RayTracingBufferTable");
		STRING_HASH_CONSTEXPR StringHash RayTracingTexture2DTableHash = StringHash::Construct("RayTracingTexture2DTable");

		for (size_t i = 0; i < resourceBindings.size(); ++i)
		{
			auto& resourceBinding = resourceBindings.at(i);

			if (resourceBinding.hash != GlobalsStringHash)
			{
				auto& foundBinding = foundResourceBindings.emplace_back();
				foundBinding.hash = resourceBinding.hash;
				foundBinding.name = resourceBinding.binding.name;
				foundBinding.value = false;
			}
		}

		for (const auto& parameter : parameterStructMetadataDesc->GetParameterMetadata())
		{
			switch (parameter.parameterType)
			{
				case ShaderParameterType::BufferSRV:
				case ShaderParameterType::BufferUAV:
				case ShaderParameterType::TextureSRV:
				case ShaderParameterType::TextureUAV:
				case ShaderParameterType::UniformBuffer:
				case ShaderParameterType::Sampler:
				case ShaderParameterType::AccelerationStructure:
				{
					for (auto& foundBinding : foundResourceBindings)
					{
						if (foundBinding.hash == parameter.hashedName)
						{
							foundBinding.value = true;
						}
					}
					break;
				}
				case ShaderParameterType::ResourceTable:
				{
					for (auto& foundBinding : foundResourceBindings)
					{
						if (foundBinding.hash == RayTracingBufferTableHash)
						{
							foundBinding.value = true;
						}

						if (foundBinding.hash == RayTracingTexture2DTableHash)
						{
							foundBinding.value = true;
						}
					}

					break;
				}
			}
		}

		std::string errorMessage;
		bool shouldError = false;

		for (const auto& foundBinding : foundResourceBindings)
		{
			if (!foundBinding.value)
			{
				shouldError = true;
				errorMessage += std::format("{}\n", foundBinding.name);
			}
		}

		if (shouldError)
		{
			std::string error = std::format("Not all bindings were found in shader parameter struct!\n{}", errorMessage);
			VT_ENSURE_MSG(false, error);
		}
	}
}
