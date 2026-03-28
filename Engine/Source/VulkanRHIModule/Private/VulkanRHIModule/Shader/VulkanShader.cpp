#include "vkpch.h"

#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <FileSystemModule/FileUtility.h>

#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <filesystem>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanShader::VulkanShader(const ShaderCreateInfo& createInfo)
		: m_name(createInfo.name), m_failureIsFatal(createInfo.failureIsFatal)
	{
		VT_ENSURE(!createInfo.sourceFilepath.IsEmpty());
		VT_ENSURE(!createInfo.entryPoint.empty());

		m_sourceInfo.sourceEntry.entryPoint = createInfo.entryPoint;
		m_sourceInfo.sourceEntry.filepath = createInfo.sourceFilepath;
		m_sourceInfo.sourceEntry.shaderStage = createInfo.stage;
		m_permutationConfig = createInfo.permutationConfig;

		LoadAndCompileShader(createInfo.forceCompile);
	}

	VulkanShader::VulkanShader(const ShaderCreateInfo& createInfo, const String& source)
		: m_name(createInfo.name), m_failureIsFatal(createInfo.failureIsFatal)
	{
		VT_ENSURE(!source.empty());
		VT_ENSURE(!createInfo.entryPoint.empty());

		m_sourceInfo.source = source;
		m_sourceInfo.sourceEntry.entryPoint = createInfo.entryPoint;
		m_sourceInfo.sourceEntry.shaderStage = createInfo.stage;
		m_permutationConfig = createInfo.permutationConfig;

		LoadAndCompileShader(createInfo.forceCompile);
	}

	VulkanShader::~VulkanShader()
	{
		Release();
	}
	
	StringView VulkanShader::GetName() const
	{
		return m_name;
	}
	
	size_t VulkanShader::GetHash() const
	{
		return m_hash;
	}
	
	bool VulkanShader::IsValid() const
	{
		return m_shaderModule != nullptr;
	}
	
	ShaderStage VulkanShader::GetShaderStage() const
	{
		return m_sourceInfo.sourceEntry.shaderStage;
	}
	
	void* VulkanShader::GetHandleImpl() const
	{
		return m_shaderModule;
	}

	void VulkanShader::LoadAndCompileShader(bool forceCompile)
	{
		if (!m_sourceInfo.sourceEntry.filepath.IsEmpty())
		{
			if (!FileUtility::ReadStringFromFile(m_sourceInfo.sourceEntry.filepath, m_sourceInfo.source))
			{
				return;
			}
		}
	
		if (m_sourceInfo.source.empty())
		{
			VT_LOGC(Error, LogVulkanRHI, "Filepath for shader {} not found!", m_name);
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
				VT_ENSURE_MSG(false, FormatString("Shader {} failed to compile!", m_name));
			}
			return;
		}

		m_shaderInfo.outputFormats = compilationResult.outputFormats;
		m_shaderInfo.vertexLayout = compilationResult.vertexLayout;
		m_shaderInfo.instanceLayout = compilationResult.instanceLayout;

		m_shaderParameterMap = compilationResult.shaderParameterMap;
		m_shaderIncludeDependencies = compilationResult.includeDependencies;

		// Release old shader
		Release();

		// Create shader module
		CreateShader(compilationResult.shaderBinary);
		GenerateHash();

		// Clean up
		if (!m_sourceInfo.sourceEntry.filepath.IsEmpty())
		{
			m_sourceInfo.source.clear();
		}
	}

	void VulkanShader::Release()
	{
		if (m_shaderModule)
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyShaderModule(device->GetHandle<VkDevice>(), m_shaderModule, VT_VULKAN_ALLOCATOR);
			m_shaderModule = nullptr;
		}
	}

	void VulkanShader::CreateShader(const Vector<uint32_t>& shaderBinary)
	{
		VkShaderModuleCreateInfo moduleInfo{};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = shaderBinary.size() * sizeof(uint32_t);
		moduleInfo.pCode = shaderBinary.data();

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateShaderModule(device->GetHandle<VkDevice>(), &moduleInfo, VT_VULKAN_ALLOCATOR, &m_shaderModule));
	}

	void VulkanShader::GenerateHash()
	{
		m_hash = std::hash<const void*>()(m_shaderModule);
	}

	void VulkanShader::Reload(bool forceCompile /* = false */)
	{
		LoadAndCompileShader(forceCompile);
	}
}
