#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorManager.h"
#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Graphics/PhysicalDeviceProperties.h"

#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/RHIFeatures.h>

#include <CoreUtilities/ConsoleVariableRegistry.h>
#include <CoreUtilities/MemoryUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	static ConsoleVariable<int32_t> g_rhiVulkanBindlessMaxResourceDescriptors(
		"rhi.Vulkan.Bindless.MaxResourceDescriptors",
		1'000'000,
		"The max number of bindless resource descriptors"
	);

	static ConsoleVariable<int32_t> g_rhiVulkanBindlessMaxSamplerDescriptors(
		"rhi.Vulkan.Bindless.MaxSamplerDescriptors",
		1024,
		"The max number of bindless sampler descriptors"
	);

	VulkanBindlessDescriptorManager::VulkanBindlessDescriptorManager()
	{
		VT_ASSERT(s_instance == nullptr);
		s_instance = this;

		Initialize();
	}

	VulkanBindlessDescriptorManager::~VulkanBindlessDescriptorManager()
	{
		s_instance = nullptr;

		Release();
	}

	void VulkanBindlessDescriptorManager::Initialize()
	{
		m_availableResourceBindlessIndices.Allocate(g_rhiVulkanBindlessMaxResourceDescriptors.GetValue());
		m_availableSamplerBindlessIndices.Allocate(g_rhiVulkanBindlessMaxSamplerDescriptors.GetValue());

		for (int32_t i = g_rhiVulkanBindlessMaxResourceDescriptors.GetValue() - 1; i >= 0; --i)
		{
			m_availableResourceBindlessIndices.Push(BindlessIndex(static_cast<uint32_t>(i)));
		}

		for (int32_t i = g_rhiVulkanBindlessMaxSamplerDescriptors.GetValue() - 1; i >= 0; --i)
		{
			m_availableSamplerBindlessIndices.Push(BindlessIndex(static_cast<uint32_t>(i)));
		}

		CreateBindlessDescriptorSetLayout();
		CreateDescriptorBuffer();
	}

	void VulkanBindlessDescriptorManager::Release()
	{
		if (m_descriptorBufferPtr)
		{
			m_descriptorBuffer->Unmap();
			m_descriptorBufferPtr = nullptr;
		}
	}

	BindlessIndex VulkanBindlessDescriptorManager::AllocateIndex()
	{
		BindlessIndex result;
		m_availableResourceBindlessIndices.Pop(result);

		return result;
	}

	void VulkanBindlessDescriptorManager::FreeIndex(BindlessIndex index)
	{
		m_availableResourceBindlessIndices.Push(index);
	}

	BindlessIndex VulkanBindlessDescriptorManager::AllocateSamplerIndex()
	{
		BindlessIndex result;
		m_availableSamplerBindlessIndices.Pop(result);

		return result;
	}

	void VulkanBindlessDescriptorManager::FreeSamplerIndex(BindlessIndex index)
	{
		m_availableSamplerBindlessIndices.Push(index);
	}

	void VulkanBindlessDescriptorManager::CreateBindlessDescriptorSetLayout()
	{
		constexpr uint32_t CBV_SRV_UAV_Binding = 0;
		constexpr uint32_t SamplersBinding = 1;
		constexpr VkDescriptorType SamplerDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

		Vector<VkDescriptorType> heapDescriptorTypes =
		{
			VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
		};


		if (RHICanUseRayTracing())
		{
			heapDescriptorTypes.emplace_back(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);
		}

		Vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
		Vector<VkMutableDescriptorTypeListEXT> descriptorTypeLists;

		{
			VkMutableDescriptorTypeListEXT& descriptorTypeList = descriptorTypeLists.emplace_back();
			descriptorTypeList.descriptorTypeCount = static_cast<uint32_t>(heapDescriptorTypes.size());
			descriptorTypeList.pDescriptorTypes = heapDescriptorTypes.data();
		}

		{
			VkMutableDescriptorTypeListEXT& descriptorTypeList = descriptorTypeLists.emplace_back();
			descriptorTypeList.descriptorTypeCount = 1;
			descriptorTypeList.pDescriptorTypes = &SamplerDescriptorType;
		}

		VkMutableDescriptorTypeCreateInfoEXT mutableDescriptorCreateInfo{};
		mutableDescriptorCreateInfo.sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT;
		mutableDescriptorCreateInfo.pNext = nullptr;
		mutableDescriptorCreateInfo.mutableDescriptorTypeListCount = static_cast<uint32_t>(descriptorTypeLists.size());
		mutableDescriptorCreateInfo.pMutableDescriptorTypeLists = descriptorTypeLists.data();

		{
			VkDescriptorSetLayoutBinding& cbvSrvUavBinding = descriptorSetLayoutBindings.emplace_back();
			cbvSrvUavBinding.binding = CBV_SRV_UAV_Binding;
			cbvSrvUavBinding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
			cbvSrvUavBinding.descriptorCount = g_rhiVulkanBindlessMaxResourceDescriptors.GetValue();
			cbvSrvUavBinding.pImmutableSamplers = nullptr;
			cbvSrvUavBinding.stageFlags = VK_SHADER_STAGE_ALL;
		}

		{
			VkDescriptorSetLayoutBinding& samplersBinding = descriptorSetLayoutBindings.emplace_back();
			samplersBinding.binding = SamplersBinding;
			samplersBinding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
			samplersBinding.descriptorCount = g_rhiVulkanBindlessMaxSamplerDescriptors.GetValue();
			samplersBinding.pImmutableSamplers = nullptr;
			samplersBinding.stageFlags = VK_SHADER_STAGE_ALL;
		}

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = nullptr;
		layoutCreateInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size());
		layoutCreateInfo.pBindings = descriptorSetLayoutBindings.data();
		layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		Vector<VkDescriptorBindingFlags> bindingFlags{};

		VkDescriptorSetLayoutBindingFlagsCreateInfo extendedInfo{};
		extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		extendedInfo.pNext = &mutableDescriptorCreateInfo;
		extendedInfo.bindingCount = layoutCreateInfo.bindingCount;

		constexpr VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

		for (const auto& binding : descriptorSetLayoutBindings)
		{
			VT_UNUSED(binding);

			auto& flags = bindingFlags.emplace_back();
			flags = bindlessFlags;
		}

		extendedInfo.pBindingFlags = bindingFlags.data();
		layoutCreateInfo.pNext = &extendedInfo;

		VkDescriptorSetLayoutSupport support{};
		support.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_SUPPORT;
		support.pNext = nullptr;

		auto vkDevice = GraphicsContext::GetDevice()->GetHandle<VkDevice>();

		vkGetDescriptorSetLayoutSupport(vkDevice, &layoutCreateInfo, &support);

		VT_ENSURE(support.supported);
		VT_VK_CHECK(vkCreateDescriptorSetLayout(vkDevice, &layoutCreateInfo, VT_VULKAN_ALLOCATOR, &m_bindlessDescriptorSetLayout));
	
		vkGetDescriptorSetLayoutSizeEXT(vkDevice, m_bindlessDescriptorSetLayout, &m_descriptorLayoutSize);
		m_descriptorLayoutSize = ::Utility::Align(m_descriptorLayoutSize, g_physicalDeviceProperties.descriptorBufferProperties.descriptorBufferOffsetAlignment);

		vkGetDescriptorSetLayoutBindingOffsetEXT(vkDevice, m_bindlessDescriptorSetLayout, CBV_SRV_UAV_Binding, &m_resourceDescriptorOffset);
		vkGetDescriptorSetLayoutBindingOffsetEXT(vkDevice, m_bindlessDescriptorSetLayout, SamplersBinding, &m_samplerDescriptorOffset);
	}

	void VulkanBindlessDescriptorManager::CreateDescriptorBuffer()
	{
		const PhysicalDeviceDescriptorBufferPropertiesEXT& descriptorBufferProperties = g_physicalDeviceProperties.descriptorBufferProperties;

		m_descriptorSize = 0;
		m_descriptorSize = std::max(descriptorBufferProperties.sampledImageDescriptorSize, descriptorBufferProperties.combinedImageSamplerDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.storageImageDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.uniformTexelBufferDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.storageTexelBufferDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.uniformBufferDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.storageBufferDescriptorSize);
		m_descriptorSize = std::max(m_descriptorSize, descriptorBufferProperties.accelerationStructureDescriptorSize);

		BufferDesc bufferDesc{};
		bufferDesc.elementSize = m_descriptorLayoutSize;
		bufferDesc.numElements = 1;
		bufferDesc.usage = BufferUsage::DescriptorBuffer | BufferUsage::DeviceAddress;
		bufferDesc.memoryUsage = MemoryUsage::CPUToGPU;
		bufferDesc.debugName = "BindlessDescriptorBuffer";

		m_descriptorBuffer = GraphicsContext::Get().GetDefaultAllocator()->CreateBuffer(bufferDesc);
		m_descriptorBufferPtr = m_descriptorBuffer->Map<uint8_t>();
	}

	void VulkanBindlessDescriptorManager::UpdateDescriptor(BindlessIndex index, const VkDescriptorGetInfoEXT& descriptorGetInfo, uint64_t descriptorSize)
	{
		auto vkDevice = GraphicsContext::GetDevice()->GetHandle<VkDevice>();
		
		const uint64_t descriptorOffset = m_resourceDescriptorOffset + index.Get() * m_descriptorSize;
		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, descriptorSize, m_descriptorBufferPtr + descriptorOffset);
	}

	void VulkanBindlessDescriptorManager::UpdateSamplerDescriptor(BindlessIndex index, const VkDescriptorGetInfoEXT& descriptorGetInfo, uint64_t descriptorSize)
	{
		auto vkDevice = GraphicsContext::GetDevice()->GetHandle<VkDevice>();

		const uint64_t descriptorOffset = m_samplerDescriptorOffset + index.Get() * g_physicalDeviceProperties.descriptorBufferProperties.samplerDescriptorSize;
		vkGetDescriptorEXT(vkDevice, &descriptorGetInfo, descriptorSize, m_descriptorBufferPtr + descriptorOffset);
	}
}
