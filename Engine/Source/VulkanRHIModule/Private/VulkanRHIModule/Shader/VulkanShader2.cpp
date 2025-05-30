#include "vkpch.h"

#include "VulkanRHIModule/Shader/VulkanShader2.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Shader/ShaderUtility.h>
#include <RHIModule/Shader/ShaderCompiler.h>
#include <RHIModule/Graphics/GraphicsContext.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanShader2::VulkanShader2(const ShaderCreateInfo& createInfo)
		: m_name(createInfo.name)
	{
		VT_ENSURE(!createInfo.sourceFilepath.empty());
		VT_ENSURE(!createInfo.entryPoint.empty());

		m_sourceInfo.sourceEntry.entryPoint = createInfo.entryPoint;
		m_sourceInfo.sourceEntry.filepath = createInfo.sourceFilepath;
		m_sourceInfo.sourceEntry.shaderStage = createInfo.stage;
		m_permutationConfig = createInfo.permutationConfig;

		LoadAndCompileShader();
	}

	VulkanShader2::~VulkanShader2()
	{
		Release();
	}
	
	std::string_view VulkanShader2::GetName() const
	{
		return m_name;
	}
	
	size_t VulkanShader2::GetHash() const
	{
		return size_t();
	}
	
	bool VulkanShader2::IsValid() const
	{
		return m_shaderModule != nullptr;
	}
	
	ShaderStage VulkanShader2::GetShaderStage() const
	{
		return m_sourceInfo.sourceEntry.shaderStage;
	}
	
	void* VulkanShader2::GetHandleImpl() const
	{
		return m_shaderModule;
	}

	void VulkanShader2::LoadAndCompileShader()
	{
		m_sourceInfo.source = Utility::ReadStringFromFile(m_sourceInfo.sourceEntry.filepath);
	
		if (m_sourceInfo.source.empty())
		{
			// #TODO_Ivar: Handle this gracefully in some way.
			return;
		}

		ShaderCompiler::Specification2 compileSpec;
		compileSpec.forceCompile = false;
		compileSpec.shaderSourceInfo = m_sourceInfo;
		compileSpec.permutationConfig = m_permutationConfig;

		const ShaderCompiler::CompilationResultData2 compilationResult = ShaderCompiler::TryCompile2(compileSpec);
		if (compilationResult.result != ShaderCompiler::CompilationResult::Success)
		{
			// #TODO_Ivar: Handle
			return;
		}

		m_shaderInfo.outputFormats = compilationResult.outputFormats;
		m_shaderInfo.vertexLayout = compilationResult.vertexLayout;
		m_shaderInfo.instanceLayout = compilationResult.instanceLayout;
		m_shaderInfo.shaderUniforms = compilationResult.shaderUniforms;
		m_shaderInfo.bindings = compilationResult.bindings;
		m_bindings.uniformBuffers = compilationResult.uniformBuffers;
		m_bindings.storageBuffers = compilationResult.storageBuffers;
		m_bindings.images = compilationResult.images;
		m_bindings.samplers = compilationResult.samplers;

		m_shaderParameterMap = compilationResult.shaderParameterMap;

		// Release old shader
		Release();

		// Create shader module
		CreateShader(compilationResult.shaderBinary);
		GenerateHash();
	}

	void VulkanShader2::Release()
	{
		if (m_shaderModule)
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyShaderModule(device->GetHandle<VkDevice>(), m_shaderModule, nullptr);
			m_shaderModule = nullptr;
		}
	}

	void VulkanShader2::CreateShader(const Vector<uint32_t>& shaderBinary)
	{
		VkShaderModuleCreateInfo moduleInfo{};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = shaderBinary.size() * sizeof(uint32_t);
		moduleInfo.pCode = shaderBinary.data();

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateShaderModule(device->GetHandle<VkDevice>(), &moduleInfo, nullptr, &m_shaderModule));
	}

	void VulkanShader2::GenerateHash()
	{
		m_hash = std::hash<const void*>()(m_shaderModule);
	}
}
