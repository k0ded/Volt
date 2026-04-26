#include "rcpch.h"

#include "RenderCore/Shader/IORequestCompileShader.h"

namespace Volt
{
	IORequestCompileShader::IORequestCompileShader(StringView name, const RHI::ShaderCreateInfo& createInfo)
		: IORequest(name),
		m_createInfo(createInfo),
		m_resultCode(IORequestResultCode::Undefined)
	{}

	IORequestCompileShader::IORequestCompileShader(StringView name, const RHI::ShaderCreateInfo& createInfo, const String& source)
		: IORequest(name),
		m_createInfo(createInfo),
		m_source(source),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	void IORequestCompileShader::Execute()
	{
		VT_PROFILE_FUNCTION();

		if (m_source.empty())
		{
			m_resultShader = RHI::Shader::Create(m_createInfo);
		}
		else
		{
			m_resultShader = RHI::Shader::CreateWithSource(m_createInfo, m_source);
		}

		if (m_resultShader == nullptr)
		{
			m_resultCode = IORequestResultCode::Failure;
		}
		else
		{
			m_resultCode = IORequestResultCode::Success;
		}
	}

	IORequestResultCode IORequestCompileShader::GetResultCode() const
	{
		return m_resultCode;
	}

	IORequestCompileShader_Multiple::IORequestCompileShader_Multiple(StringView name, Vector<RHI::ShaderCreateInfo>&& createInfos)
		: IORequest(name),
		m_createInfos(std::move(createInfos)),
		m_resultCode(IORequestResultCode::Undefined)
	{
	}

	void IORequestCompileShader_Multiple::Execute()
	{
		VT_PROFILE_FUNCTION();

		for (const RHI::ShaderCreateInfo& createInfo : m_createInfos)
		{
			IntRef<RHI::Shader> shader = RHI::Shader::Create(createInfo);
			if (shader)
			{
				m_resultShaders.emplace_back(shader, createInfo.permutationConfig.GetPermutationIndex());
			}
		}

		m_resultCode = m_resultShaders.empty() ? IORequestResultCode::Failure : IORequestResultCode::Success;
	}

	IORequestResultCode IORequestCompileShader_Multiple::GetResultCode() const
	{
		return m_resultCode;
	}
}
