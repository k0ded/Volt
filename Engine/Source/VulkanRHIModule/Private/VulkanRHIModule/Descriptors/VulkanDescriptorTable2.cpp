#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorTable2.h"
#include "VulkanRHIModule/Pipelines/VulkanComputePipeline2.h"
#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline2.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"

#include <RHIModule/Images/ImageView.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Graphics/GraphicsContext.h>
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

	VulkanDescriptorTable2::VulkanDescriptorTable2(const DescriptorTableCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		VT_ENSURE(m_createInfo.computePipeline || m_createInfo.renderPipeline);

		Invalidate();
	}

	VulkanDescriptorTable2::~VulkanDescriptorTable2()
	{
		Release();
	}
	
	void VulkanDescriptorTable2::SetImageView(RawPtr<ImageView> imageView, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		// Note: This might cause trouble, the maps might not have stable pointers.
		auto& imageDescriptor = m_imageDescriptorInfos[set][binding][arrayIndex];
		imageDescriptor.imageView = imageView->GetHandle<VkImageView>();
		imageDescriptor.sampler = nullptr;

		VT_ENSURE(imageDescriptor.imageView);

		uint32_t writeDescriptorIndex = 0;

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding][arrayIndex].value == DefaultInvalid::INVALID_VALUE)
		{
			writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite& writeDescriptorCopy = m_activeDescriptorWrites.emplace_back() = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = arrayIndex;
			writeDescriptorCopy.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&imageDescriptor);

			writeDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding][arrayIndex].value = writeDescriptorIndex;
		}
		else
		{
			writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding][arrayIndex].value;
			m_activeDescriptorWrites.at(writeDescriptorIndex).pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&imageDescriptor);
		}

		imageDescriptor.imageLayout = Utility::GetImageLayoutFromDescriptorType(static_cast<VkDescriptorType>(m_activeDescriptorWrites.at(writeDescriptorIndex).descriptorType));
	}
	
	void VulkanDescriptorTable2::SetBufferView(RawPtr<BufferView> bufferView, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		VulkanBufferView& vkBufferView = bufferView->AsRef<VulkanBufferView>();

		auto& bufferDescriptor = m_bufferDescriptorInfos[set][binding][arrayIndex];
		bufferDescriptor.buffer = vkBufferView.GetHandle<VkBuffer>();

		VT_ENSURE(bufferDescriptor.buffer);

		auto bufferResource = vkBufferView.GetResource();
		if (bufferResource->GetType() == ResourceType::StorageBuffer)
		{
			bufferDescriptor.range = bufferResource->GetByteSize();
		}

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding][arrayIndex].value == DefaultInvalid::INVALID_VALUE)
		{
			const uint32_t writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite& writeDescriptorCopy = m_activeDescriptorWrites.emplace_back() = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = arrayIndex;
			writeDescriptorCopy.pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo*>(&bufferDescriptor);

			const uint32_t activeWriteDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding][arrayIndex].value = activeWriteDescriptorIndex;
		}
		else
		{
			const uint32_t writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding][arrayIndex].value;
			m_activeDescriptorWrites.at(writeDescriptorIndex).pBufferInfo = reinterpret_cast<const VkDescriptorBufferInfo*>(&bufferDescriptor);
		}
	}
	
	void VulkanDescriptorTable2::SetSamplerState(RawPtr<SamplerState> samplerState, uint32_t set, uint32_t binding, uint32_t arrayIndex)
	{
		// Make sure set and binding is actually used in the pipeline.
		if (!m_writeDescriptorsMapping.contains(set) || !m_writeDescriptorsMapping.at(set).contains(binding))
		{
			// We return here without error as it is fine to do this. Maybe add later under validation define?
			return;
		}

		m_isDirty = true;

		auto& samplerDescriptor = m_imageDescriptorInfos[set][binding][arrayIndex];
		samplerDescriptor.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		samplerDescriptor.imageView = nullptr;
		samplerDescriptor.sampler = samplerState->GetHandle<VkSampler>();

		// Create a new active descriptor write, or use a cached one.
		if (m_activeDescriptorWritesMapping[set][binding][arrayIndex].value == DefaultInvalid::INVALID_VALUE)
		{
			const uint32_t writeDescriptorIndex = m_writeDescriptorsMapping[set][binding];

			DescriptorWrite writeDescriptorCopy = m_descriptorWrites.at(writeDescriptorIndex);
			writeDescriptorCopy.dstArrayElement = arrayIndex;
			writeDescriptorCopy.pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&samplerDescriptor);
			m_activeDescriptorWrites.emplace_back(writeDescriptorCopy);

			const uint32_t activeWriteDescriptorIndex = static_cast<uint32_t>(m_activeDescriptorWrites.size() - 1);
			m_activeDescriptorWritesMapping[set][binding][arrayIndex].value = activeWriteDescriptorIndex;
		}
		else
		{
			const uint32_t writeDescriptorIndex = m_activeDescriptorWritesMapping[set][binding][arrayIndex].value;
			m_activeDescriptorWrites.at(writeDescriptorIndex).pImageInfo = reinterpret_cast<const VkDescriptorImageInfo*>(&samplerDescriptor);
		}
	}
	
	void VulkanDescriptorTable2::SetImageView(std::string_view name, RawPtr<ImageView> view, uint32_t arrayIndex)
	{
	}
	
	void VulkanDescriptorTable2::SetBufferView(std::string_view name, RawPtr<BufferView> view, uint32_t arrayIndex)
	{
	}
	
	void VulkanDescriptorTable2::SetSamplerState(std::string_view name, RawPtr<SamplerState> samplerState, uint32_t arrayIndex)
	{
	}
	
	void VulkanDescriptorTable2::PrepareForRender()
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
	
	void VulkanDescriptorTable2::Bind(CommandBuffer& commandBuffer)
	{
		VT_PROFILE_FUNCTION();

		// No descriptor sets, nothing to do.
		if (m_descriptorSets.empty())
		{
			return;
		}

		VulkanCommandBuffer& vulkanCommandBuffer = commandBuffer.AsRef<VulkanCommandBuffer>();
		const VkPipelineBindPoint bindPoint = m_createInfo.computePipeline ? VK_PIPELINE_BIND_POINT_COMPUTE : VK_PIPELINE_BIND_POINT_GRAPHICS;

		PrepareForRender();

		VkPipelineLayout pipelineLayout = nullptr;
		if (m_createInfo.computePipeline)
		{
			pipelineLayout = m_createInfo.computePipeline->AsRef<VulkanComputePipeline2>().GetPipelineLayout();
		}
		else
		{
			pipelineLayout = m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline2>().GetPipelineLayout();
		}

		vkCmdBindDescriptorSets(vulkanCommandBuffer.GetHandle<VkCommandBuffer>(), bindPoint, pipelineLayout, 0, static_cast<uint32_t>(m_descriptorSets.size()), m_descriptorSets.data(), 0, nullptr);
	}

	void VulkanDescriptorTable2::Invalidate()
	{
		Release();
	
		if (m_createInfo.computePipeline)
		{
			CreateFromComputePipeline();
		}
		else
		{
			CreateFromRenderPipeline();
		}
	}

	void VulkanDescriptorTable2::Release()
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

	void VulkanDescriptorTable2::CreateFromComputePipeline()
	{
		VulkanComputePipeline2& vulkanPipeline = m_createInfo.computePipeline->AsRef<VulkanComputePipeline2>();

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

		for (const auto& setLayout : descriptorSetLayouts)
		{
			allocInfo.pSetLayouts = &setLayout;
			VT_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle<VkDevice>(), &allocInfo, &m_descriptorSets.emplace_back()));
		}

		BuildWriteDescriptors();
		InitializeInfoStructs();
	}

	void VulkanDescriptorTable2::CreateFromRenderPipeline()
	{
		VulkanRenderPipeline2& vulkanPipeline = m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline2>();

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

		for (const auto& setLayout : descriptorSetLayouts)
		{
			allocInfo.pSetLayouts = &setLayout;
			VT_VK_CHECK(vkAllocateDescriptorSets(device->GetHandle<VkDevice>(), &allocInfo, &m_descriptorSets.emplace_back()));
		}

		BuildWriteDescriptors();
		InitializeInfoStructs();
	}
	
	void* VulkanDescriptorTable2::GetHandleImpl() const
	{
		return nullptr;
	}

	void VulkanDescriptorTable2::BuildWriteDescriptors()
	{
		m_descriptorWrites.clear();
		m_activeDescriptorWrites.clear();
		m_writeDescriptorsMapping.clear();

		const ShaderBindings* shaderBindings = nullptr;
	
		if (m_createInfo.computePipeline)
		{
			shaderBindings = &m_createInfo.computePipeline->AsRef<VulkanComputePipeline2>().GetBindings();
		}
		else
		{
			shaderBindings = &m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline2>().GetBindings();
		}

		// Cache all possible descriptor writes, to skip that during runtime.
		for (const auto& [set, bindings] : shaderBindings->uniformBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				InitializeWriteDescriptor(writeDescriptor, binding, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER), m_descriptorSets[set]);
				m_writeDescriptorsMapping[set][binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}

		for (const auto& [set, bindings] : shaderBindings->storageBuffers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				InitializeWriteDescriptor(writeDescriptor, binding, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER), m_descriptorSets[set]);
				m_writeDescriptorsMapping[set][binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}

		for (const auto& [set, bindings] : shaderBindings->storageImages)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				InitializeWriteDescriptor(writeDescriptor, binding, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE), m_descriptorSets[set]);
				m_writeDescriptorsMapping[set][binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}

		for (const auto& [set, bindings] : shaderBindings->images)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				InitializeWriteDescriptor(writeDescriptor, binding, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE), m_descriptorSets[set]);
				m_writeDescriptorsMapping[set][binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}

		for (const auto& [set, bindings] : shaderBindings->samplers)
		{
			for (const auto& [binding, data] : bindings)
			{
				auto& writeDescriptor = m_descriptorWrites.emplace_back();
				InitializeWriteDescriptor(writeDescriptor, binding, static_cast<uint32_t>(VK_DESCRIPTOR_TYPE_SAMPLER), m_descriptorSets[set]);
				m_writeDescriptorsMapping[set][binding] = static_cast<uint32_t>(m_descriptorWrites.size() - 1);
			}
		}
	}

	void VulkanDescriptorTable2::InitializeWriteDescriptor(DescriptorWrite& writeDescriptor, const uint32_t binding, const uint32_t descriptorType, VkDescriptorSet_T* dstDescriptorSet)
	{
		writeDescriptor.sType = static_cast<uint32_t>(VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
		writeDescriptor.pNext = nullptr;
		writeDescriptor.descriptorCount = 1;
		writeDescriptor.dstArrayElement = 0;
		writeDescriptor.dstBinding = binding;
		writeDescriptor.descriptorType = descriptorType;
		writeDescriptor.dstSet = dstDescriptorSet;
	}

	void VulkanDescriptorTable2::InitializeInfoStructs()
	{
		const ShaderBindings* shaderBindings = nullptr;

		if (m_createInfo.computePipeline)
		{
			shaderBindings = &m_createInfo.computePipeline->AsRef<VulkanComputePipeline2>().GetBindings();
		}
		else
		{
			shaderBindings = &m_createInfo.renderPipeline->AsRef<VulkanRenderPipeline2>().GetBindings();
		}

		// Initialize structs used during descriptor update.
		for (const auto& [set, bindings] : shaderBindings->uniformBuffers)
		{
			for (const auto& [binding, info] : bindings)
			{
				m_bufferDescriptorInfos[set][binding][0].offset = 0;
				m_bufferDescriptorInfos[set][binding][0].range = info.size;
			}
		}

		for (const auto& [set, bindings] : shaderBindings->storageBuffers)
		{
			for (const auto& [binding, info] : bindings)
			{
				m_bufferDescriptorInfos[set][binding][0].offset = 0;
				m_bufferDescriptorInfos[set][binding][0].range = info.size;
			}
		}
	}
}
