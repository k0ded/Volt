#include "vrpch.h"
#include "RenderMaterial.h"

#include <RHIModule/Shader/Shader2.h>
#include <RHIModule/Pipelines/ComputePipeline.h>

#include <CoreUtilities/Math/Hash.h>

namespace Volt
{
	RenderMaterial::RenderMaterial(const std::string& name)
		: m_name(name)
	{
		GenerateHash();
	}

	RenderMaterial::RenderMaterial(const std::string& name, RefPtr<RHI::Shader2> shader)
		: m_name(name)
	{
		VT_ENSURE(shader);
		m_shader = shader;
		m_pipeline = RHI::ComputePipeline::Create(shader);
		GenerateHash();
	}

	void RenderMaterial::SetTexture(uint32_t index, RenderTexture resource)
	{
		if (m_textures.size() < static_cast<size_t>(index))
		{
			m_textures.resize(index + 1);
		}

		m_textures[index] = resource;
		m_isDirty = true;
	}

	void RenderMaterial::SetTextures(const PagedVector<RenderTexture>& textures)
	{
		m_textures = textures;
		m_isDirty = true;
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
			RHI::ShaderCreateInfo shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.sourceFilepath = filepath;
			shaderSpecification.forceCompile = true;

			m_shader = RHI::Shader2::Create(shaderSpecification);
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
    }

	void RenderMaterial::GenerateHash()
	{
		m_hash = UUID64();
	}
}
