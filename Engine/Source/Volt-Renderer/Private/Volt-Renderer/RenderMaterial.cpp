#include "vrpch.h"
#include "RenderMaterial.h"

#include <RHIModule/Shader/Shader.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Math/Hash.h>

namespace Volt
{
	RenderMaterial::RenderMaterial(const std::string& name)
		: m_name(name)
	{
	}

	RenderMaterial::RenderMaterial(const std::string& name, RefPtr<RHI::Shader> shader)
		: m_name(name)
	{
		VT_ENSURE(shader);
		m_shader = shader;
		m_pipeline = RHI::ComputePipeline::Create(shader);
	}

	void RenderMaterial::SetTexture(uint32_t index, RenderTexture resource)
	{
		if (m_textures.size() < static_cast<size_t>(index))
		{
			m_textures.resize(index + 1);
		}

		m_textures[index] = resource;
		m_isDirty = true;
		GenerateHash();
	}

	void RenderMaterial::SetTextures(const PagedVector<RenderTexture>& textures)
	{
		m_textures = textures;
		m_isDirty = true;
		GenerateHash();
	}

	bool RenderMaterial::DoMaterialRequireUpdate() const
	{
		return m_isDirty;
	}

	void RenderMaterial::ClearStatus()
	{
		m_isDirty = false;
	}

	void RenderMaterial::Invalidate(const std::filesystem::path& filepath)
    {
		if (!m_shader)
		{
			RHI::ShaderSpecification shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.sourceEntries = { { "main", RHI::ShaderStage::Compute, filepath} };
			shaderSpecification.forceCompile = true;

			m_shader = RHI::Shader::Create(shaderSpecification);
		}
		else
		{
			m_shader->Reload(false);
		}

		if (!m_pipeline)
		{
			m_pipeline = RHI::ComputePipeline::Create(m_shader);
		}
		else
		{
			m_pipeline->Invalidate();
		}

		GenerateHash();
    }

	void RenderMaterial::GenerateHash()
	{
		m_hash = m_pipeline->GetHash();
		m_hash = Math::HashCombine(m_hash, std::hash<std::string>()(m_name));
		for (const auto& texture : m_textures)
		{
			m_hash = Math::HashCombine(m_hash, std::hash<uint32_t>()(texture.GetResource()));
		}
	}
}
