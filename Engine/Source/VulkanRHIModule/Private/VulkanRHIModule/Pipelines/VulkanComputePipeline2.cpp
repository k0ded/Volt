#include "vkpch.h"

#include "VulkanRHIModule/Pipelines/VulkanComputePipeline2.h"
#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/Shader/VulkanShader2.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/Shader/Shader.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Hash.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanComputePipeline2::VulkanComputePipeline2(RefPtr<Shader2> shader)
		: m_shader(shader)
	{
		Invalidate();
	}

	VulkanComputePipeline2::~VulkanComputePipeline2()
	{
		Release();
	}
	
	void VulkanComputePipeline2::Invalidate()
	{
		Release();
		
		VT_ENSURE(m_shader);
		VT_ENSURE(m_shader->GetShaderStage() == ShaderStage::Compute);
		
		ScopedTimer scopedTimer{};

		auto device = GraphicsContext::GetDevice();

		// Create descriptor set layouts
		{
			DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
			m_descriptorSetLayouts = descriptorSetLayoutBuilder.BuildFromShaderBindings(m_shader->GetBindings());
			m_descriptorPoolSizes = descriptorSetLayoutBuilder.CalculateDescriptorPoolSizesFromBindings(m_shader->GetBindings());

			m_pipelineBindings = m_shader->GetBindings();
		}

		// Create pipeline layout
		{
			VkPipelineLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
			info.pSetLayouts = m_descriptorSetLayouts.data();
			info.pushConstantRangeCount = 0;
			info.pPushConstantRanges = nullptr;

			VT_VK_CHECK(vkCreatePipelineLayout(device->GetHandle<VkDevice>(), &info, nullptr, &m_pipelineLayout));
		}

		// Create pipeline
		{
			VulkanShader2& vulkanShader = m_shader->AsRef<VulkanShader2>();
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
	
	RefPtr<Shader> VulkanComputePipeline2::GetShader() const
	{
		return nullptr;
	}
	
	bool VulkanComputePipeline2::IsValid() const
	{
		return m_pipeline != nullptr;
	}
	
	size_t VulkanComputePipeline2::GetHash() const
	{
		return m_hash;
	}
	
	void* VulkanComputePipeline2::GetHandleImpl() const
	{
		return m_pipeline;
	}
	
	void VulkanComputePipeline2::Release()
	{
		if (!m_pipeline)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([pipeline = m_pipeline, pipelineLayout = m_pipelineLayout, descriptorSetLayouts = m_descriptorSetLayouts]()
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

	void VulkanComputePipeline2::GenerateHash()
	{
		m_hash = m_shader->GetHash();

		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipeline)));
		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipelineLayout)));
	}
}
