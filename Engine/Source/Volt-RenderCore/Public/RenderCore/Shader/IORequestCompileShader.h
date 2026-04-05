#pragma once

#include <FileSystemModule/IOThreads/IORequest.h>

#include <RenderCore/Config.h>

#include <RHIModule/Shader/Shader.h>

namespace Volt
{
	class IORequestCompileShader : public IORequest
	{
	public:
		using ResultType = IntRef<RHI::Shader>;

		VTRC_API IORequestCompileShader(StringView name, const RHI::ShaderCreateInfo& createInfo);
		VTRC_API IORequestCompileShader(StringView name, const RHI::ShaderCreateInfo& createInfo, const String& source);
		~IORequestCompileShader() override = default;

		void Execute() override;
		IORequestResultCode GetResultCode() const override;

		IntRef<RHI::Shader>& GetResult() { return m_resultShader; }

	private:
		RHI::ShaderCreateInfo m_createInfo;
		String m_source;

		IntRef<RHI::Shader> m_resultShader;
		IORequestResultCode m_resultCode;
	};
}
