#include "vkpch.h"

#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline.h"
#include "VulkanRHIModule/Pipelines/StaticSamplerDescriptorSetManager.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/RayTracing/RayTracingTableDescriptorSetManager.h"
#include "VulkanRHIModule/VulkanResourceCast.h"
#include "VulkanRHIModule/Utility/PushConstantsBuilder.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/RHIFeatures.h>
#include <RHIModule/Shader/ShaderUtility.h>

#include <CoreUtilities/Time/ScopedTimer.h>
#include <CoreUtilities/Math/Hash.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	struct VertexAttributeData
	{
		Vector<VkVertexInputBindingDescription> bindingDescriptions;
		Vector<VkVertexInputAttributeDescription> attributeDescriptions;
	};

	inline VertexAttributeData CreateVertexLayout(const BufferLayoutMap& vertexLayoutMap, const BufferLayout& instanceLayout, VertexBufferLayout& vertexBufferLayout)
	{
		VertexAttributeData result{};

		uint32_t lastVertexBufferIndex = 0;
		uint32_t attributeIndex = 0;

		for (const auto& [index, vertexLayout] : vertexLayoutMap)
		{
			VkVertexInputBindingDescription& bindingDesc = result.bindingDescriptions.emplace_back();
			bindingDesc.binding = index;
			bindingDesc.stride = vertexLayout.GetStride();
			bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			for (const auto& element : vertexLayout.GetElements())
			{
				VkVertexInputAttributeDescription& desc = result.attributeDescriptions.emplace_back();
				desc.binding = index;
				desc.location = attributeIndex++;
				desc.format = Utility::VoltToVulkanElementFormat(element.type);
				desc.offset = static_cast<uint32_t>(element.offset);
			}

			lastVertexBufferIndex = std::max(lastVertexBufferIndex, index);

			vertexBufferLayout.vertexBuffers.emplace_back(vertexLayout, index);
		}

		if (instanceLayout.IsValid())
		{
			lastVertexBufferIndex++;
			vertexBufferLayout.perInstanceVertexBuffer = { instanceLayout, lastVertexBufferIndex };

			VkVertexInputBindingDescription& instanceBindingDesc = result.bindingDescriptions.emplace_back();
			instanceBindingDesc.binding = lastVertexBufferIndex;
			instanceBindingDesc.stride = instanceLayout.GetStride();
			instanceBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

			for (const auto& element : instanceLayout.GetElements())
			{
				VkVertexInputAttributeDescription& desc = result.attributeDescriptions.emplace_back();
				desc.binding = lastVertexBufferIndex;
				desc.location = attributeIndex++;
				desc.format = Utility::VoltToVulkanElementFormat(element.type);
				desc.offset = static_cast<uint32_t>(element.offset);
			}
		}

		return result;
	}

	inline VertexAttributeData CreateVertexLayoutFromShaders(const Vector<RefPtr<Shader>>& shaders, VertexBufferLayout& vertexBufferLayout)
	{
		// We will pick the first shader that contains a vertex layout (should only be one anyways)
		for (const auto shader : shaders)
		{
			const ShaderInfo& shaderInfo = shader->GetShaderInfo();

			if (!shaderInfo.vertexLayout.empty())
			{
				if (shaderInfo.vertexLayout.begin()->second.IsValid())
				{
					return CreateVertexLayout(shaderInfo.vertexLayout, shaderInfo.instanceLayout, vertexBufferLayout);
				}
			}
		}

		// We allow no vertex layout
		return {};
	}

	VulkanRenderPipeline::VulkanRenderPipeline(const RenderPipelineCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Invalidate();
	}

	VulkanRenderPipeline::~VulkanRenderPipeline()
	{
		Release();
	}

	void VulkanRenderPipeline::Invalidate()
	{
		Release();

		ScopedTimer scopedTimer{};

		if (m_createInfo.enablePrimitiveRestart)
		{
			VT_ENSURE(m_createInfo.topology != Topology::TriangleList && m_createInfo.topology != Topology::LineList && m_createInfo.topology != Topology::PatchList && m_createInfo.topology != Topology::PointList);
		}

		VerifyShaderStages();

		auto device = GraphicsContext::GetDevice();

		// Create descriptor set layouts
		bool anyAccessesRayTracingResourceTable = false;
		{
			Vector<ShaderParameterMap::ResourceBindings> shaderResourceBindings;

			for (const auto shader : m_createInfo.shaders)
			{
				const ShaderParameterMap& parameterMap = shader->GetParameterMap();

				m_shaderParameterMaps[GetDescriptorSetIndexFromShaderStage(shader->GetShaderStage())] = parameterMap;
				shaderResourceBindings.emplace_back(parameterMap.GetResourceBindings());
			
				anyAccessesRayTracingResourceTable |= parameterMap.AccessesRayTracingTable();
			}

			DescriptorSetLayoutBuilder descriptorSetLayoutBuilder;
			m_descriptorSets = descriptorSetLayoutBuilder.BuildFromShaderResourceBindings(shaderResourceBindings, anyAccessesRayTracingResourceTable);
		}

		// Create pipeline layout
		{
			PushConstantsBuilder pushConstantsBuilder{};
			VkPushConstantRange pushConstantRange = pushConstantsBuilder.BuildPushConstantRange(m_shaderParameterMaps);
			const bool hasPushConstantRange = pushConstantRange.size > 0;

			VT_ENSURE_MSG(pushConstantRange.size <= 128, "Larger than 128 bytes is not allowed, since 128 bytes is the vulkan specification guaranteed amount.");

			VkPipelineLayoutCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			info.pNext = nullptr;
			info.setLayoutCount = static_cast<uint32_t>(m_descriptorSets.pipelineLayoutDescriptorSetLayouts.size());
			info.pSetLayouts = m_descriptorSets.pipelineLayoutDescriptorSetLayouts.data();
			info.pushConstantRangeCount = hasPushConstantRange ? 1 : 0;
			info.pPushConstantRanges = hasPushConstantRange ? &pushConstantRange : nullptr;

			VT_VK_CHECK(vkCreatePipelineLayout(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &m_pipelineLayout));
		}

		// Create pipeline
		{
			VertexAttributeData vertexAttributes = CreateVertexLayoutFromShaders(m_createInfo.shaders, m_vertexBufferLayout);

			VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexAttributes.bindingDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = vertexAttributes.bindingDescriptions.data();

			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributes.attributeDescriptions.size());
			vertexInputInfo.pVertexAttributeDescriptions = vertexAttributes.attributeDescriptions.data();

			VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
			inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyInfo.topology = Utility::VoltToVulkanTopology(m_createInfo.topology);
			inputAssemblyInfo.primitiveRestartEnable = m_createInfo.enablePrimitiveRestart ? VK_TRUE : VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportInfo{};
			viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportInfo.viewportCount = 1;
			viewportInfo.pViewports = nullptr;
			viewportInfo.scissorCount = 1;
			viewportInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
			rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerInfo.depthClampEnable = m_createInfo.enableDepthClamp ? VK_TRUE : VK_FALSE;
			rasterizerInfo.depthBiasEnable = m_createInfo.depthBiasClamp > 0.f ? VK_TRUE : VK_FALSE;
			rasterizerInfo.depthBiasClamp = m_createInfo.depthBiasClamp;
			rasterizerInfo.depthBiasConstantFactor = m_createInfo.depthBiasConstantFactor;
			rasterizerInfo.depthBiasSlopeFactor = m_createInfo.depthBiasSlopeFactor;
			rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerInfo.polygonMode = Utility::VoltToVulkanFill(m_createInfo.fillMode);
			rasterizerInfo.cullMode = Utility::VoltToVulkanCull(m_createInfo.cullMode);
			rasterizerInfo.lineWidth = 1.f;
			rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;

			// #TODO_Ivar: Add tessellation support

			VkPipelineMultisampleStateCreateInfo multiSampleInfo{};
			multiSampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multiSampleInfo.sampleShadingEnable = VK_FALSE;
			multiSampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendStateCreateInfo blendInfo{};
			blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendInfo.logicOpEnable = VK_FALSE;
			blendInfo.logicOp = VK_LOGIC_OP_COPY;
			blendInfo.blendConstants[0] = 0.f;
			blendInfo.blendConstants[1] = 0.f;
			blendInfo.blendConstants[2] = 0.f;
			blendInfo.blendConstants[3] = 0.f;

			Vector<VkPipelineColorBlendAttachmentState> blendAttachments{};

			const Vector<PixelFormat> shaderOutputFormats = Utility::GetOutputFormatsFromShaders(m_createInfo.shaders);
			for (size_t index = 0; const auto& outputFormat : shaderOutputFormats)
			{
				if (Utility::IsDepthFormat(outputFormat) || Utility::IsStencilFormat(outputFormat))
				{
					continue;
				}

				VkPipelineColorBlendAttachmentState& blendAttachment = blendAttachments.emplace_back();
				blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
				blendAttachment.blendEnable = m_createInfo.attachmentBlendStates[index].enabled;
				blendAttachment.srcColorBlendFactor = Utility::VoltToVulkanBlendFactor(m_createInfo.attachmentBlendStates[index].srcColorBlend);
				blendAttachment.srcAlphaBlendFactor = Utility::VoltToVulkanBlendFactor(m_createInfo.attachmentBlendStates[index].srcAlphaBlend);
				blendAttachment.dstColorBlendFactor = Utility::VoltToVulkanBlendFactor(m_createInfo.attachmentBlendStates[index].dstColorBlend);
				blendAttachment.dstAlphaBlendFactor = Utility::VoltToVulkanBlendFactor(m_createInfo.attachmentBlendStates[index].dstAlphaBlend);
				blendAttachment.colorBlendOp = Utility::VoltToVulkanBlendOp(m_createInfo.attachmentBlendStates[index].colorBlendOp);
				blendAttachment.alphaBlendOp = Utility::VoltToVulkanBlendOp(m_createInfo.attachmentBlendStates[index].colorBlendOp);

				index++;
			}

			blendInfo.attachmentCount = static_cast<uint32_t>(blendAttachments.size());
			blendInfo.pAttachments = blendAttachments.data();

			VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
			depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilInfo.depthCompareOp = Utility::VoltToVulkanCompareOp(m_createInfo.depthCompareOperator);
			depthStencilInfo.stencilTestEnable = VK_FALSE;
			depthStencilInfo.depthBoundsTestEnable = VK_FALSE;

			switch (m_createInfo.depthMode)
			{
				case DepthMode::None:
				{
					depthStencilInfo.depthTestEnable = VK_FALSE;
					depthStencilInfo.depthWriteEnable = VK_FALSE;
					break;
				}

				case DepthMode::Read:
				{
					depthStencilInfo.depthTestEnable = VK_TRUE;
					depthStencilInfo.depthWriteEnable = VK_FALSE;
					break;
				}

				case DepthMode::Write:
				{
					depthStencilInfo.depthTestEnable = VK_FALSE;
					depthStencilInfo.depthWriteEnable = VK_TRUE;
					break;
				}

				case DepthMode::ReadWrite:
				{
					depthStencilInfo.depthTestEnable = VK_TRUE;
					depthStencilInfo.depthWriteEnable = VK_TRUE;
					break;
				}
			}

			VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
			dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

			Vector<VkDynamicState> dynamicStates =
			{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR,
			};

			dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
			dynamicStateInfo.pDynamicStates = dynamicStates.data();

			Vector<PixelFormat> outputFormats{};
			PixelFormat depthFormat = PixelFormat::UNDEFINED;
			PixelFormat stencilFormat = PixelFormat::UNDEFINED;

			for (const auto& format : shaderOutputFormats)
			{
				if (Utility::IsDepthFormat(format))
				{
					depthFormat = format;
				}
				else if (Utility::IsStencilFormat(format))
				{
					stencilFormat = format;
				}
				else
				{
					outputFormats.emplace_back(format);
				}
			}

			VkPipelineRenderingCreateInfo pipelineRenderingInfo{};
			pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
			pipelineRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(outputFormats.size());
			pipelineRenderingInfo.pColorAttachmentFormats = reinterpret_cast<const VkFormat*>(outputFormats.data());
			pipelineRenderingInfo.depthAttachmentFormat = static_cast<VkFormat>(depthFormat);
			pipelineRenderingInfo.stencilAttachmentFormat = static_cast<VkFormat>(stencilFormat);

			Vector<VkPipelineShaderStageCreateInfo> pipelineStageInfos{};

			for (const auto shader : m_createInfo.shaders)
			{
				VulkanShader& vulkanShader = shader->AsRef<VulkanShader>();

				auto& newStageInfo = pipelineStageInfos.emplace_back();
				newStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				newStageInfo.pNext = nullptr;
				newStageInfo.stage = Utility::VoltToVulkanShaderStage(shader->GetShaderStage());
				newStageInfo.module = vulkanShader.GetShaderModule();
				newStageInfo.pName = vulkanShader.GetShaderSourceInfo().sourceEntry.entryPoint.c_str();
			}

			VkGraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.pNext = &pipelineRenderingInfo;
			pipelineInfo.stageCount = static_cast<uint32_t>(pipelineStageInfos.size());
			pipelineInfo.pStages = pipelineStageInfos.data();
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
			pipelineInfo.pViewportState = &viewportInfo;
			pipelineInfo.pRasterizationState = &rasterizerInfo;
			pipelineInfo.pMultisampleState = &multiSampleInfo;
			pipelineInfo.pColorBlendState = &blendInfo;
			pipelineInfo.pDepthStencilState = &depthStencilInfo;
			pipelineInfo.pDynamicState = &dynamicStateInfo;
			pipelineInfo.pTessellationState = nullptr;
			pipelineInfo.layout = m_pipelineLayout;
			pipelineInfo.subpass = 0;
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

			VT_VK_CHECK(vkCreateGraphicsPipelines(device->GetHandle<VkDevice>(), VK_NULL_HANDLE, 1, &pipelineInfo, VT_VULKAN_ALLOCATOR, &m_pipeline));
		}

		if (RHI::RHICanUseRayTracing() && anyAccessesRayTracingResourceTable)
		{
			// Erase the ray tracing pipelines from the lists, as they should not be accessed outside of the pipeline.
			if (m_descriptorSets.descriptorSetLayouts.contains(RayTracingTableDescriptorSetManager::Set))
			{
				m_descriptorSets.descriptorSetLayouts.erase(RayTracingTableDescriptorSetManager::Set);
			}

			for (auto it = m_descriptorSets.pipelineLayoutDescriptorSetLayouts.begin(); it != m_descriptorSets.pipelineLayoutDescriptorSetLayouts.end(); ++it)
			{
				if (*it == RayTracingTableDescriptorSetManager::Get().GetDescriptorSetLayout())
				{
					m_descriptorSets.pipelineLayoutDescriptorSetLayouts.erase(it);
					break;
				}
			}
		}

		GenerateHash();
		VT_LOGC(Trace, LogVulkanRHI, "Created Vulkan Render Pipeline in {} seconds!", scopedTimer.GetTime<Time::Seconds>());
	}

	bool VulkanRenderPipeline::IsValid() const
	{
		return m_pipeline != nullptr;
	}

	size_t VulkanRenderPipeline::GetHash() const
	{
		return m_hash;
	}

	void* VulkanRenderPipeline::GetHandleImpl() const
	{
		return m_pipeline;
	}

	void VulkanRenderPipeline::Release()
	{
		if (!m_pipeline)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([pipeline = m_pipeline, pipelineLayout = m_pipelineLayout, descriptorSetLayouts = m_descriptorSets.pipelineLayoutDescriptorSetLayouts]()
		{
			VulkanGraphicsContext* vulkanGraphicsContext = ResourceCast(&GraphicsContext::Get());
			auto device = GraphicsContext::GetDevice();

			vkDestroyPipeline(device->GetHandle<VkDevice>(), pipeline, VT_VULKAN_ALLOCATOR);
			vkDestroyPipelineLayout(device->GetHandle<VkDevice>(), pipelineLayout, VT_VULKAN_ALLOCATOR);

			// Make sure we don't destroy any shared descriptor set layouts.
			for (const auto& descriptorSetLayout : descriptorSetLayouts)
			{
				if (descriptorSetLayout != StaticSamplerDescriptorSetManager::Get().GetDescriptorSetLayout() &&
					descriptorSetLayout != vulkanGraphicsContext->GetEmptyDescriptorSetLayout())
				{
					vkDestroyDescriptorSetLayout(device->GetHandle<VkDevice>(), descriptorSetLayout, VT_VULKAN_ALLOCATOR);
				}
			}
		});

		m_pipeline = nullptr;
		m_pipelineLayout = nullptr;
		m_descriptorSets.pipelineLayoutDescriptorSetLayouts.clear();
	}

	void VulkanRenderPipeline::GenerateHash()
	{
		m_hash = 0;
		for (const auto& shader : m_createInfo.shaders)
		{
			m_hash = Math::HashCombine(m_hash, shader->GetHash());
		}

		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipeline)));
		m_hash = Math::HashCombine(m_hash, std::hash<void*>()(static_cast<void*>(m_pipelineLayout)));
	}

	void VulkanRenderPipeline::VerifyShaderStages()
	{
		bool foundVertexShader = false;
		bool foundMeshShader = false;
		bool foundAmplificationShader = false;
		bool foundPixelShader = false;

		for (const auto& shader : m_createInfo.shaders)
		{
			switch (shader->GetShaderStage())
			{
				case ShaderStage::Vertex: VT_ENSURE(!foundVertexShader); foundVertexShader = true; break;
				case ShaderStage::Mesh: VT_ENSURE(!foundMeshShader); foundMeshShader = true; break;
				case ShaderStage::Amplification: VT_ENSURE(!foundAmplificationShader); foundAmplificationShader = true; break;
				case ShaderStage::Pixel: VT_ENSURE(!foundPixelShader); foundPixelShader = true; break;
			}
		}

		if (foundPixelShader)
		{
			VT_ENSURE(foundVertexShader || foundMeshShader);
		}

		if (foundAmplificationShader)
		{
			VT_ENSURE(foundMeshShader && !foundVertexShader);
		}

		if (foundVertexShader)
		{
			VT_ENSURE(!foundAmplificationShader && !foundMeshShader);
		}

		if (foundMeshShader)
		{
			VT_ENSURE(!foundVertexShader);
		}
	}

	const ShaderResourceBinding* VulkanRenderPipeline::GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const
	{
		auto& shaderParameterMap = m_shaderParameterMaps[GetDescriptorSetIndexFromShaderStage(shaderStage)];

		const ShaderParameterMap::ResourceBindings& resourceBindingsMap = shaderParameterMap.GetResourceBindings();
		for (const auto& [binding, nameHash] : resourceBindingsMap)
		{
			if (nameHash == name)
			{
				return &binding;

				// We can break here because there is only max one of each shader stage per pipeline
				break;
			}
		}

		return nullptr;
	}

	ArrayView<ShaderParameterMap> VulkanRenderPipeline::GetShaderParameterMaps() const
	{
		return m_shaderParameterMaps;
	}

	const Vector<RefPtr<Shader>>& VulkanRenderPipeline::GetShaders() const
	{
		return m_createInfo.shaders;
	}

	const VertexBufferLayout& VulkanRenderPipeline::GetVertexBufferLayout() const
	{
		return m_vertexBufferLayout;
	}
}
