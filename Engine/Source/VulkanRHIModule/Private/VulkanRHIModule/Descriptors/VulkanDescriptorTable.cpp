#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorTable.h"
#include "VulkanRHIModule/Pipelines/VulkanComputePipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"

#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Globals.h>
#include <RHIModule/RHIModule.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	namespace Utility
	{
		inline VkImageLayout GetImageLayoutFromDescriptorType(VkDescriptorType descriptorType)
		{
			switch (descriptorType)
			{
				case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_IMAGE_LAYOUT_GENERAL;
			}

			return VK_IMAGE_LAYOUT_UNDEFINED;
		}
	}

	VulkanDescriptorTable::VulkanDescriptorTable(const DescriptorTableCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		VT_ENSURE(m_createInfo.computePipeline || m_createInfo.renderPipeline);

		Invalidate();
	}

	VulkanDescriptorTable::~VulkanDescriptorTable()
	{
		Release();
	}
	
	void VulkanDescriptorTable::SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		auto& imageDescriptor = m_imageDescriptorInfos[set][binding];
		imageDescriptor.imageView = imageView->GetHandle<VkImageView>();
		imageDescriptor.sampler = nullptr;

		VT_ENSURE(imageDescriptor.imageView);

		uint32_t writeDescriptorIndex = 0;

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding].value == DefaultInvalid::INVALID_VALUE)
		{
			writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite& writeDescriptorCopy = m_activeDescriptorWrites.emplace_back() = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = 0;
			writeDescriptorCopy.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&imageDescriptor);

			writeDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding].value = writeDescriptorIndex;
		}
		else
		{
			writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding].value;
			m_activeDescriptorWrites.at(writeDescriptorIndex).pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&imageDescriptor);
		}

		imageDescriptor.imageLayout = Utility::GetImageLayoutFromDescriptorType(static_cast<VkDescriptorType>(m_activeDescriptorWrites.at(writeDescriptorIndex).descriptorType));
	}
	
	void VulkanDescriptorTable::SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		VulkanBufferView& vkBufferView = bufferView->AsRef<VulkanBufferView>();
		const bool isTexelBufferView = vkBufferView.IsTexelBufferView();

		DescriptorBufferInfo* bufferDescriptorPtr = nullptr;
		VkBufferView* vkTexelBufferView = nullptr;

		if (!isTexelBufferView)
		{
			auto& bufferDescriptor = m_bufferDescriptorInfos[set][binding];
			bufferDescriptor.buffer = vkBufferView.GetHandle<VkBuffer>();
			bufferDescriptor.range = vkBufferView.GetDesc().size;
			bufferDescriptor.offset = vkBufferView.GetDesc().offset;
		
			bufferDescriptorPtr = &bufferDescriptor;

			VT_ENSURE(bufferDescriptor.buffer);
		}
		else
		{
			auto& view = m_texelBufferViews[set][binding];
			view = vkBufferView.GetTexelBufferView();

			vkTexelBufferView = &view;
		}

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding].value == DefaultInvalid::INVALID_VALUE)
		{
			const uint32_t writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite& writeDescriptorCopy = m_activeDescriptorWrites.emplace_back() = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = 0;

			if (!isTexelBufferView)
			{
				writeDescriptorCopy.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo*>(bufferDescriptorPtr);
			}
			else
			{
				writeDescriptorCopy.pTexelBufferView = vkTexelBufferView;
			}

			const uint32_t activeWriteDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding].value = activeWriteDescriptorIndex;
		}
		else
		{
			const uint32_t writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding].value;
			auto& activeDescriptorWrite = m_activeDescriptorWrites.at(writeDescriptorIndex);

			if (!isTexelBufferView)
			{
				activeDescriptorWrite.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo*>(bufferDescriptorPtr);
			}
			else
			{
				activeDescriptorWrite.pTexelBufferView = vkTexelBufferView;
			}
		}
	}
	
	void VulkanDescriptorTable::SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		auto& samplerDescriptor = m_imageDescriptorInfos[set][binding];
		samplerDescriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		samplerDescriptor.imageView = nullptr;
		samplerDescriptor.sampler = samplerState->GetHandle<VkSampler>();

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding].value == DefaultInvalid::INVALID_VALUE)
		{
			const uint32_t writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite writeDescriptorCopy = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = 0;
			writeDescriptorCopy.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&samplerDescriptor);
			m_activeDescriptorWrites.emplace_back(writeDescriptorCopy);

			const uint32_t activeWriteDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding].value = activeWriteDescriptorIndex;
		}
		else
		{
			const uint32_t writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding].value;
			m_activeDescriptorWrites.at(writeDescriptorIndex).pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&samplerDescriptor);
		}
	}
	
	void VulkanDescriptorTable::PrepareForRender()
	{
		VT_PROFILE_FUNCTION();

		if (!m_isDirty)
		{
			return;
		}

		if (m_activeDescriptorWrites.empty())
		{
			return;
		}

		auto device = GraphicsContext::GetDevice();
		const VkWriteDescriptorSet* writeDescriptorsPtr = reinterpret_cast<const VkWriteDescriptorSet*>(m_activeDescriptorWrites.data());

		vkUpdateDescriptorSets(device->GetHandle<VkDevice>(), static_cast<uint32_t>(m_activeDescriptorWrites.size()), writeDescriptorsPtr, 0, nullptr);

		m_activeDescriptorWrites.clear();
		m_activeDescriptorWritesMapping.clear();

		m_isDirty = false;
	}

	void VulkanDescriptorTable::Invalidate()
	{
		Release();

		VT_ENSURE(m_createInfo.computePipeline || m_createInfo.renderPipeline);
	
		if (m_createInfo.computePipeline)
		{
			CreateFromComputePipeline();
		}
		else
		{
			CreateFromRenderPipeline();
		}
	}

	void VulkanDescriptorTable::Release()
	{
		if (!m_descriptorPool)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([descriptorPool = m_descriptorPool]()
		{
			auto device = GraphicsContext::GetDevice();
			vkDestroyDescriptorPool(device->GetHandle<VkDevice>(), descriptorPool, nullptr);
		});

		m_descriptorPool = nullptr;
	}

	void VulkanDescriptorTable::CreateFromComputePipeline()
	{
		VulkanComputePipeline& vulkanPipeline = m_createInfo.computePipeline->AsRef<VulkanComputePipeline>();

		const auto& descriptorPoolSizes = vulkanPipeline.GetDescriptorPoolSizes();
		const auto& descriptorSetLayouts = vulkanPipeline.GetDescriptorSetLayouts();

		// A pipeline can be without descriptor sets, in that case we don't do anything
		if (descriptorSetLayouts.empty())
		{
			return;
		}

		Vector<VkDescriptorPoolSize> poolSizes{};
		for (const auto& [type, count] : descriptorPoolSizes)
		{
			poolSizes.emplace_back(static_cast<VkDescriptorType>(type), count);
		}

		// Create descriptor pool
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = static_cast<uint32_t>(descriptorSetLayouts.size());
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateDescriptorPool(device->GetHandle<VkDevice>(), &poolInfo, nullptr, &m_descriptorPool));

		// Allocate all descriptor sets
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorSetCount = 1;
		allocInfo.descriptorPool = m_descriptorPool;

		for (const auto& [setIndex, setLayout] : descriptorSetLayouts)
		{
			allocInfo.pSetLayouts = &setLayout;
			VT_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle<VkDevice>(), &allocInfo, &m_descriptorSets[setIndex]));
		}

		BuildDescriptorInfos();
		BuildWriteDescriptors();
	}

	void VulkanDescriptorTable::CreateFromRenderPipeline()
	{
		VulkanRenderPipeline& vulkanPipeline = m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline>();

		const auto& descriptorPoolSizes = vulkanPipeline.GetDescriptorPoolSizes();
		const auto& descriptorSetLayouts = vulkanPipeline.GetDescriptorSetLayouts();

		// A pipeline can be without descriptor sets, in that case we don't do anything
		if (descriptorSetLayouts.empty())
		{
			return;
		}

		Vector<VkDescriptorPoolSize> poolSizes{};
		for (const auto& [type, count] : descriptorPoolSizes)
		{
			poolSizes.emplace_back(static_cast<VkDescriptorType>(type), count);
		}

		// Create descriptor pool
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.flags = 0;
		poolInfo.maxSets = static_cast<uint32_t>(descriptorSetLayouts.size());
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		auto device = GraphicsContext::GetDevice();
		VT_VK_CHECK(vkCreateDescriptorPool(device->GetHandle<VkDevice>(), &poolInfo, nullptr, &m_descriptorPool));

		// Allocate all descriptor sets
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pNext = nullptr;
		allocInfo.descriptorSetCount = 1;
		allocInfo.descriptorPool = m_descriptorPool;

		for (const auto& [setIndex, setLayout] : descriptorSetLayouts)
		{
			allocInfo.pSetLayouts = &setLayout;
			VT_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle<VkDevice>(), &allocInfo, &m_descriptorSets[setIndex]));
		}

		BuildDescriptorInfos();
		BuildWriteDescriptors();
	}
	
	void* VulkanDescriptorTable::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanDescriptorTable::BuildWriteDescriptors()
	{
		m_descriptorWrites.clear();
		m_activeDescriptorWrites.clear();
		m_writeDescriptorsMapping.clear();

		Vector<ShaderParameterMap> shaderParameterMaps;
	
		if (m_createInfo.computePipeline)
		{
			shaderParameterMaps.emplace_back(m_createInfo.computePipeline->GetShaderParameterMap());
		}
		else
		{
			shaderParameterMaps = m_createInfo.renderPipeline->GetShaderParameterMaps();
		}

		// Cache all possible descriptor writes, to skip that during runtime.
		for (const auto& parameterMap : shaderParameterMaps)
		{
			for (const auto& [nameHash, binding] : parameterMap.GetResourceBindings())
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				
				VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
				switch (binding.registerType)
				{
					case ShaderRegisterType::CBV: descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
					case ShaderRegisterType::Sampler: descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; break;
					case ShaderRegisterType::SRV: 
					{
						switch (binding.resourceType)
						{
							case ShaderResourceType::StructuredBuffer: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
							case ShaderResourceType::TexelBuffer: descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; break;
							case ShaderResourceType::Texture: descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
						}
						break;
					}

					case ShaderRegisterType::UAV:
					{
						switch (binding.resourceType)
						{
							case ShaderResourceType::StructuredBuffer: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
							case ShaderResourceType::TexelBuffer: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; break;
							case ShaderResourceType::Texture: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
						}
						break;
					}
				}

				InitializeWriteDescriptor(writeDescriptor, binding.binding, static_cast<uint32_t>(descriptorType), m_descriptorSets.at(binding.set));
				m_writeDescriptorsMapping[binding.set][binding.binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}
	}

	void VulkanDescriptorTable::InitializeWriteDescriptor(DescriptorWrite& writeDescriptor, const uint32_t binding, const uint32_t descriptorType, VkDescriptorSet_T* dstDescriptorSet)
	{
		writeDescriptor.sType = static_cast<uint32_t>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
		writeDescriptor.pNext = nullptr;
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;
		writeDescriptor.dstBinding = binding;
		writeDescriptor.descriptorType = descriptorType;
		writeDescriptor.dstSet = dstDescriptorSet;
	}

	VkPipelineLayout_T* VulkanDescriptorTable::GetRelatedPipelineLayout() const
	{
		if (m_createInfo.computePipeline)
		{
			return m_createInfo.computePipeline->AsRef<VulkanComputePipeline>().GetPipelineLayout();
		}
		else
		{
			return m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline>().GetPipelineLayout();
		}
	}

	uint32_t VulkanDescriptorTable::GetRelatedBindPoint() const
	{
		const VkPipelineBindPoint bindPoint = m_createInfo.computePipeline ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;
		return bindPoint;
	}

	size_t VulkanDescriptorTable::GetHash() const
	{
		if (m_createInfo.computePipeline)
		{
			return m_createInfo.computePipeline->GetHash();
		}
		else
		{
			return m_createInfo.renderPipeline->GetHash();
		}
	}

	void VulkanDescriptorTable::BuildDescriptorInfos()
	{
		m_imageDescriptorInfos.clear();
		m_bufferDescriptorInfos.clear();
		
		Vector<ShaderParameterMap> shaderParameterMaps;

		if (m_createInfo.computePipeline)
		{
			shaderParameterMaps.emplace_back(m_createInfo.computePipeline->GetShaderParameterMap());
		}
		else
		{
			shaderParameterMaps = m_createInfo.renderPipeline->GetShaderParameterMaps();
		}

		// Cache all possible descriptor infos.
		for (const auto& parameterMap : shaderParameterMaps)
		{
			for (const auto& [nameHash, binding] : parameterMap.GetResourceBindings())
			{
				switch (binding.resourceType)
				{
					case ShaderResourceType::Texture:
					{
						auto& descriptorInfo = m_imageDescriptorInfos[binding.set][binding.binding];
						descriptorInfo.imageView = nullptr;
						descriptorInfo.sampler = nullptr;

						if (binding.registerType == ShaderRegisterType::SRV)
						{
							descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
						}
						else
						{
							descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						}

						break;
					}

					case ShaderResourceType::Sampler:
					{
						auto& descriptorInfo = m_imageDescriptorInfos[binding.set][binding.binding];
						descriptorInfo.imageView = nullptr;
						descriptorInfo.sampler = nullptr;
						descriptorInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

						break;
					}

					case ShaderResourceType::StructuredBuffer:
					case ShaderResourceType::UniformBuffer:
					case ShaderResourceType::TexelBuffer:
					{
						auto& descriptorInfo = m_bufferDescriptorInfos[binding.set][binding.binding];
						descriptorInfo.buffer = nullptr;
						descriptorInfo.offset = 0;
						descriptorInfo.range = 0;

						break;
					}
				}
			}
		}
	}
}
