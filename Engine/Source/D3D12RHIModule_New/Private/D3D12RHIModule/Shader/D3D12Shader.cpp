#include "dxpch.h"

#include "D3D12RHIModule/Shader/D3D12Shader.h"

#include <RHIModule/Shader/ShaderUtility.h>
#include <RHIModule/Shader/ShaderCompiler.h>

namespace Volt::RHI
{
	D3D12Shader::D3D12Shader(const ShaderCreateInfo& createInfo)
		: m_name(createInfo.name), m_failureIsFatal(createInfo.failureIsFatal)
	{
		VT_ENSURE(!createInfo.sourceFilepath.empty());
		VT_ENSURE(!createInfo.entryPoint.empty());

		m_sourceInfo.sourceEntry.entryPoint = createInfo.entryPoint;
		m_sourceInfo.sourceEntry.filepath = createInfo.sourceFilepath;
		m_sourceInfo.sourceEntry.shaderStage = createInfo.stage;
		m_permutationConfig = createInfo.permutationConfig;

		LoadAndCompileShader(createInfo.forceCompile);
	}

	D3D12Shader::~D3D12Shader()
	{
	}

	std::string_view D3D12Shader::GetName() const
	{
		return m_name;
	}

	size_t D3D12Shader::GetHash() const
	{
		return m_hash;
	}

	bool D3D12Shader::IsValid() const
	{
		return !m_shaderBinary.empty();
	}

	ShaderStage D3D12Shader::GetShaderStage() const
	{
		return m_sourceInfo.sourceEntry.shaderStage;
	}

	void* D3D12Shader::GetHandleImpl() const
	{
		return nullptr;
	}

	void D3D12Shader::LoadAndCompileShader(bool forceCompile)
	{
		m_sourceInfo.source = Utility::ReadStringFromFile(m_sourceInfo.sourceEntry.filepath);

		if (m_sourceInfo.source.empty())
		{
			VT_LOGC(Error, LogD3D12RHI, "Filepath for shader {} not found!", m_name);
			VT_ENSURE(false);
			return;
		}

		ShaderCompiler::Specification compileSpec;
		compileSpec.forceCompile = forceCompile;
		compileSpec.shaderSourceInfo = m_sourceInfo;
		compileSpec.permutationConfig = m_permutationConfig;

		const ShaderCompiler::CompilationResultData compilationResult = ShaderCompiler::TryCompile(compileSpec);
		if (compilationResult.result != ShaderCompiler::CompilationResult::Success)
		{
			if (m_failureIsFatal)
			{
				VT_ENSURE_MSG(false, std::format("Shader {} failed to compile!", m_name));
			}
			return;
		}

		m_shaderInfo.outputFormats = compilationResult.outputFormats;
		m_shaderInfo.vertexLayout = compilationResult.vertexLayout;
		m_shaderInfo.instanceLayout = compilationResult.instanceLayout;

		m_shaderParameterMap = compilationResult.shaderParameterMap;
		m_shaderIncludeDependencies = compilationResult.includeDependencies;

		m_shaderBinary = compilationResult.shaderBinary;

		// Create shader module
		GenerateHash();
	}

	void D3D12Shader::GenerateHash()
	{
		m_hash = std::hash<const void*>()(m_shaderBinary.data());
	}

	void D3D12Shader::Reload(bool forceCompile /* = false */)
	{
		LoadAndCompileShader(forceCompile);
	}
}
