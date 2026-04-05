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

		m_resultCode = IORequestResultCode::Success;
	}

	IORequestResultCode IORequestCompileShader::GetResultCode() const
	{
		return m_resultCode;
	}
}
