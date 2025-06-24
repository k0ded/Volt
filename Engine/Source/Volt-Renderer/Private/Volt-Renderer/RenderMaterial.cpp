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
		GenerateHash();
	}

	RenderMaterial::RenderMaterial(const std::string& name, RefPtr<RHI::Shader> shader)
		: m_name(name)
	{
		VT_ENSURE(shader);
		m_pixelShader = shader;
		GenerateHash();
	}

	void RenderMaterial::AddTexture(uint32_t index, const std::string& name)
	{
		m_textures[index].bindingName = name;
		m_isDirty = true;
	}

	void RenderMaterial::SetTexture(uint32_t index, RenderTexture resource)
	{
		VT_ENSURE(m_textures.contains(index));
		m_textures[index].texture = resource;
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
		if (!m_pixelShader)
		{
			RHI::ShaderCreateInfo shaderSpecification;
			shaderSpecification.name = m_name;
			shaderSpecification.sourceFilepath = filepath;
			shaderSpecification.forceCompile = true;
			shaderSpecification.entryPoint = "MainPS";
			shaderSpecification.stage = RHI::ShaderStage::Pixel;
			shaderSpecification.failureIsFatal = false;

			m_pixelShader = RHI::Shader::Create(shaderSpecification);
		}
		else
		{
			m_pixelShader->Reload(false);
		}
    }

	void RenderMaterial::GenerateHash()
	{
		m_hash = UUID64();
	}
}
