#include "vrpch.h"

#include "Volt-Renderer/Material/RenderMaterial.h"
#include "Volt-Renderer/Renderer.h"

#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Descriptors/ShaderBindingMap.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Math/Hash.h>
#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	RenderMaterial::RenderMaterial(const std::string& name)
		: m_name(name)
	{
		CompiledMaterialShaders tempMaterialShaders;
		m_shaderMap.Initialize(name, std::move(tempMaterialShaders));
		GenerateHash();
	}

	void RenderMaterial::SetTexture(uint32_t index, RenderTexture resource)
	{
		// #TODO_Ivar: Hacky, should come from the graph itself.
		m_textures[index].bindingName = StringHash::Construct("Texture_" + std::to_string(index));
		m_textures[index].texture = resource;
		m_isDirty = true;
	}

	bool RenderMaterial::DoMaterialRequireUpdate() const
	{
		return m_isDirty;
	}

	void RenderMaterial::UpdateTextures()
	{
		m_renderableTextures = m_textures;
	}

	void RenderMaterial::ClearStatus()
	{
		m_isDirty = false;
	}

	void RenderMaterial::BindToShaderBindingMap(RHI::ShaderBindingMap& shaderBindingMap, RefPtr<RHI::RenderPipeline> renderPipeline) const
	{
		VT_PROFILE_FUNCTION();

		ArrayView<RHI::ShaderParameterMap> shaderParameterMaps = renderPipeline->GetShaderParameterMaps();

		for (const RHI::ShaderParameterMap& parameterMap : shaderParameterMaps)
		{
			if (!parameterMap.HasShaderBindings())
			{
				continue;
			}

			for (const auto& [index, textureInfo] : m_renderableTextures)
			{
				const RHI::ShaderResourceBinding* resourceBinding = parameterMap.GetResourceBindingFromName(textureInfo.bindingName);
				if (resourceBinding)
				{
					RefPtr<RHI::Image> texture = textureInfo.texture.GetResource();
					if (!texture)
					{
						texture = Renderer::GetDefaultResources().white1x1;
					}

					shaderBindingMap.SetTextureSRV(parameterMap.GetShaderStage(), resourceBinding->binding, texture->GetView());
				}
			}
		}
	}

	void RenderMaterial::Invalidate(CompiledMaterialShaders&& compiledMaterialShaders)
	{
		m_shaderMap.Initialize(m_name, std::move(compiledMaterialShaders));
	}

	void RenderMaterial::GenerateHash()
	{
		m_hash = UUID64();
	}
}
