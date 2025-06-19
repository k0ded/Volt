#include "vkpch.h"

#include "VulkanRHIModule/Pipelines/VulkanComputePipeline.h"
#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Hash.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanComputePipeline::VulkanComputePipeline(RefPtr<Shader> shader)
		: m_shader(shader)
	{
		Invalidate();
	}

	VulkanComputePipeline::~VulkanComputePipeline()
	{
		Release();
	}
	
	void VulkanComputePipeline::Invalidate()
	{
		Release();
		
		VT_ENSURE(m_shader);
		VT_ENSURE(m_shader->GetShaderStage() == ShaderStage::Compute);
		
		ScopedTimer scopedTimer{};

		auto device = GraphicsContext::GetDevice();
		VulkanShader& vulkanShader = m_shader->AsRef<VulkanShader>();

		// Create descriptor set layouts
		{
			const ShaderParameterMap& shaderParameterMap = m_shader->GetParameterMap();

			DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
			DescriptorSetLayoutBuilder::DescriptorSets descriptorSets = descriptorSetLayoutBuilder.BuildFromShaderResourceBindings(shaderParameterMap.GetResourceBindings());
			m_descriptorSetLayouts = descriptorSets.descriptorSetLayouts;
			m_pipelineLayoutDescriptorSetLayouts = descriptorSets.pipelineLayoutDescriptorSetLayouts;

			m_descriptorPoolSizes = descriptorSetLayoutBuilder.CalculateDescriptorPoolSizesFromBindings(shaderParameterMap.GetResourceBindings());
			m_shaderParameterMap = shaderParameterMap;
		}

		// Create pipeline layout
		{
			VkPipelineLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.setLayoutCount = static_cast<uint32_t>(m_pipelineLayoutDescriptorSetLayouts.size());
			info.pSetLayouts = m_pipelineLayoutDescriptorSetLayouts.data();
			info.pushConstantRangeCount = 0;
			info.pPushConstantRanges = nullptr;

			VT_VK_CHECK(vkCreatePipelineLayout(device->GetHandle<VkDevice>(), &info, nullptr, &m_pipelineLayout));
		}

		// Create pipeline
		{
			const std::string entryPoint = vulkanShader.GetShaderSourceInfo().sourceEntry.entryPoint;

			VkPipelineShaderStageCreateInfo stageInfo{};
			stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stageInfo.pNext = nullptr;
			stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			stageInfo.module = vulkanShader.GetShaderModule();
			stageInfo.pName = entryPoint.c_str();

			VkComputePipelineCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			info.pNext = nullptr;
			info.layout = m_pipelineLayout;
			info.flags = 0;
			info.stage = stageInfo;
			info.basePipelineHandle = nullptr;
			info.basePipelineIndex = 0;

			VT_VK_CHECK(vkCreateComputePipelines(device->GetHandle<VkDevice>(), nullptr, 1, &info, nullptr, &m_pipeline));
		}

		GenerateHash();
		VT_LOGC(Trace, LogVulkanRHI, "Created Vulkan Compute Pipeline in {} seconds!", scopedTimer.GetTime<Time::Seconds>());
	}
	
	bool VulkanComputePipeline::IsValid() const
	{
		return m_pipeline != nullptr;
	}
	
	size_t VulkanComputePipeline::GetHash() const
	{
		return m_hash;
	}
	
	void* VulkanComputePipeline::GetHandleImpl() const
	{
		return m_pipeline;
	}
	
	void VulkanComputePipeline::Release()
	{
		if (!m_pipeline)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([pipeline = m_pipeline, pipelineLayout = m_pipelineLayout, descriptorSetLayouts = m_pipelineLayoutDescriptorSetLayouts]()
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyPipeline(device->GetHandle<VkDevice>(), pipeline, nullptr);
			vkDestroyPipelineLayout(device->GetHandle<VkDevice>(), pipelineLayout, nullptr);

			for (const auto& descriptorSetLayout : descriptorSetLayouts)
			{
				vkDestroyDescriptorSetLayout(device->GetHandle<VkDevice>(), descriptorSetLayout, nullptr);
			}
		});

		m_pipeline = nullptr;
		m_pipelineLayout = nullptr;
		m_descriptorSetLayouts.clear();
	}

	void VulkanComputePipeline::GenerateHash()
	{
		m_hash = m_shader->GetHash();

		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipeline)));
		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipelineLayout)));
	}

	const ShaderResourceBinding* VulkanComputePipeline::GetResourceBindingFromName(const StringHash& name) const
	{
		const ShaderParameterMap::ResourceBindingsMap& resourceBindings = m_shaderParameterMap.GetResourceBindings();
		if (resourceBindings.contains(name))
		{
			return &resourceBindings.at(name);
		}

		return nullptr;
	}

	RefPtr<Shader> VulkanComputePipeline::GetShader2() const
	{
		return m_shader;
	}

	const ShaderParameterMap& VulkanComputePipeline::GetShaderParameterMap() const
	{
		return m_shaderParameterMap;
	}
}
