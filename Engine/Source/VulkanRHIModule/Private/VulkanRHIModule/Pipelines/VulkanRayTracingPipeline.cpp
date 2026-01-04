#include <vkpch.h>

#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"
#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Time/ScopedTimer.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	VulkanRayTracingPipeline::VulkanRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Invalidate();
	}
	
	VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
	{
		Release();
	}
	
	void VulkanRayTracingPipeline::Invalidate()
	{
		Release();

		ScopedTimer scopedTimer{};

		Vector<VkPipelineShaderStageCreateInfo> shaderStages;
		Vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;
		Vector<std::string> entryPointNames;

		for (const auto& rayGenShader : m_createInfo.rayGenTable)
		{
			VT_ENSURE(rayGenShader->GetShaderStage() == ShaderStage::RayGen);
			
			VulkanShader& vulkanShader = rayGenShader->AsRef<VulkanShader>();

			VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pNext = nullptr;
			shaderStage.module = vulkanShader.GetShaderModule();
			shaderStage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			shaderStage.pName = vulkanShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();

			VkRayTracingShaderGroupCreateInfoKHR& shaderGroup = shaderGroups.emplace_back();
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.pNext = nullptr;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			m_rayGenData.shaders.emplace_back(rayGenShader);
		}

		for (const auto& missShader : m_createInfo.missTable)
		{
			VT_ENSURE(missShader->GetShaderStage() == ShaderStage::Miss);

			VulkanShader& vulkanShader = missShader->AsRef<VulkanShader>();

			VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pNext = nullptr;
			shaderStage.module = vulkanShader.GetShaderModule();
			shaderStage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
			shaderStage.pName = vulkanShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();

			VkRayTracingShaderGroupCreateInfoKHR& shaderGroup = shaderGroups.emplace_back();
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.pNext = nullptr;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		
			m_missData.shaders.emplace_back(missShader);
		}

		for (size_t index = 0; const auto& closestHitShader : m_createInfo.closestHitTable)
		{
			// Collect all hit shaders in one group.
			VkRayTracingShaderGroupCreateInfoKHR& hitShaderGroup = shaderGroups.emplace_back();
			hitShaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			hitShaderGroup.pNext = nullptr;
			hitShaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			hitShaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
			hitShaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			hitShaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			hitShaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			// Closest Hit, always required
			{
				VT_ENSURE(closestHitShader->GetShaderStage() == ShaderStage::ClosestHit);

				VulkanShader& vulkanClosestHitShader = closestHitShader->AsRef<VulkanShader>();

				VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
				shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStage.pNext = nullptr;
				shaderStage.module = vulkanClosestHitShader.GetShaderModule();
				shaderStage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				shaderStage.pName = vulkanClosestHitShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();

				hitShaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size() - 1);

				m_hitGroupData.shaders.emplace_back(closestHitShader);
			}

			// Any hit, optional
			if (m_createInfo.anyHitTable.size() > index)
			{
				const auto& anyHitShader = m_createInfo.anyHitTable.at(index);

				VT_ENSURE(anyHitShader->GetShaderStage() == ShaderStage::AnyHit);

				VulkanShader& vulkanAnyHitShader = anyHitShader->AsRef<VulkanShader>();

				VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
				shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStage.pNext = nullptr;
				shaderStage.module = vulkanAnyHitShader.GetShaderModule();
				shaderStage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				shaderStage.pName = vulkanAnyHitShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();
			
				hitShaderGroup.anyHitShader = static_cast<uint32_t>(shaderStages.size() - 1);

				m_hitGroupData.shaders.emplace_back(anyHitShader);
			}

			// Intersection, optional
			if (m_createInfo.intersectionTable.size() > index)
			{
				const auto& intersectionShader = m_createInfo.intersectionTable.at(index);

				VT_ENSURE(intersectionShader->GetShaderStage() == ShaderStage::Intersection);

				VulkanShader& vulkanIntersectionShader = intersectionShader->AsRef<VulkanShader>();

				VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
				shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				shaderStage.pNext = nullptr;
				shaderStage.module = vulkanIntersectionShader.GetShaderModule();
				shaderStage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				shaderStage.pName = vulkanIntersectionShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();

				hitShaderGroup.intersectionShader = static_cast<uint32_t>(shaderStages.size() - 1);

				m_hitGroupData.shaders.emplace_back(intersectionShader);
			}

			index++;
		}

		for (const auto& callableShader : m_createInfo.callableTable)
		{
			VT_ENSURE(callableShader->GetShaderStage() == ShaderStage::Callable);

			VulkanShader& vulkanShader = callableShader->AsRef<VulkanShader>();

			VkPipelineShaderStageCreateInfo& shaderStage = shaderStages.emplace_back();
			shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStage.pNext = nullptr;
			shaderStage.module = vulkanShader.GetShaderModule();
			shaderStage.stage = VK_SHADER_STAGE_CALLABLE_BIT_KHR;
			shaderStage.pName = vulkanShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();

			VkRayTracingShaderGroupCreateInfoKHR& shaderGroup = shaderGroups.emplace_back();
			shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			shaderGroup.pNext = nullptr;
			shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size() - 1);
			shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
			shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

			m_callableData.shaders.emplace_back(callableShader);
		}

		auto device = GraphicsContext::GetDevice();

		// Create pipeline layout
#if 0
		{
			VkPipelineLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.setLayoutCount = 2;
			info.pSetLayouts = descriptorSetLayouts.data();
			info.pushConstantRangeCount = 0;
			info.pPushConstantRanges = nullptr;

			VT_VK_CHECK(vkCreatePipelineLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &m_pipelineLayout));
		}
#endif

		VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		pipelineCreateInfo.pNext = nullptr;
		pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCreateInfo.pStages = shaderStages.data();
		pipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
		pipelineCreateInfo.pGroups = shaderGroups.data();
		pipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
		pipelineCreateInfo.layout = m_pipelineLayout;
		pipelineCreateInfo.flags = 0;

		VT_VK_CHECK(vkCreateRayTracingPipelinesKHR(device->GetHandle<VkDevice>(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_pipeline));

		// Get shader handles
		{
			const auto& rayTracingPipelineProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetDeviceProperties().rayTracingPipelineProperties;
			const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;

			uint32_t handleOffset = 0;
			auto GetShaderHandles = [&handleOffset, handleSize, device](VkPipeline pipeline, uint32_t handleCount)
			{
				Vector<uint8_t> result;

				if (handleCount > 0)
				{
					const uint32_t shaderHandleStorageSize = handleCount * handleSize;
					result.resize_uninitialized(shaderHandleStorageSize);

					VT_VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(device->GetHandle<VkDevice>(), pipeline, handleOffset, handleCount, shaderHandleStorageSize, result.data()));
				
					handleOffset += handleCount;
				}

				return result;
			};

			m_rayGenData.shaderHandles = GetShaderHandles(m_pipeline, static_cast<uint32_t>(m_createInfo.rayGenTable.size()));
			m_missData.shaderHandles = GetShaderHandles(m_pipeline, static_cast<uint32_t>(m_createInfo.missTable.size()));
			m_hitGroupData.shaderHandles = GetShaderHandles(m_pipeline, static_cast<uint32_t>(m_createInfo.closestHitTable.size()));
			m_callableData.shaderHandles = GetShaderHandles(m_pipeline, static_cast<uint32_t>(m_createInfo.callableTable.size()));
		}

		VT_LOGC(Trace, LogVulkanRHI, "Created Vulkan RayTracing Pipeline in {} seconds!", scopedTimer.GetTime<Time::Seconds>());
	}

	bool VulkanRayTracingPipeline::IsValid() const
	{
		return m_pipeline != nullptr;
	}

	bool VulkanRayTracingPipeline::IsShaderInPipeline(RefPtr<Shader> shader) const
	{
		for (const auto& s : m_createInfo.rayGenTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		for (const auto& s : m_createInfo.missTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		for (const auto& s : m_createInfo.closestHitTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		for (const auto& s : m_createInfo.anyHitTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		for (const auto& s : m_createInfo.intersectionTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		for (const auto& s : m_createInfo.callableTable)
		{
			if (s == shader)
			{
				return true;
			}
		}

		return false;
	}

	const ShaderUniform& VulkanRayTracingPipeline::GetRenderGraphConstants() const
	{
		static ShaderUniform s;

		return s;//m_rayGenData.shaders.front()->GetResources().renderGraphConstantsData;
	}

	void* VulkanRayTracingPipeline::GetHandleImpl() const
	{
		return m_pipeline;
	}

	void VulkanRayTracingPipeline::Release()
	{
		if (m_pipeline == nullptr)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([pipelineLayout = m_pipelineLayout, pipeline = m_pipeline]()
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyPipelineLayout(device->GetHandle<VkDevice>(), pipelineLayout, VT_VULKAN_ALLOCATOR);
			vkDestroyPipeline(device->GetHandle<VkDevice>(), pipeline, VT_VULKAN_ALLOCATOR);
		});

		m_pipelineLayout = nullptr;
		m_pipeline = nullptr;

		m_rayGenData.Clear();
		m_missData.Clear();
		m_hitGroupData.Clear();
		m_callableData.Clear();
	}
}
