#include "vkpch.h"

#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorManager.h"

#include <CoreUtilities/ConsoleVariableRegistry.h>

namespace Volt::RHI
{
	static ConsoleVariable<int32_t> g_rhiVulkanMaxBindlessDescriptors(
		"rhi.Vulkan.MaxBindlessDescriptors",
		1'000'000,
		"The max number of bindless descriptors"
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
	}

	void VulkanBindlessDescriptorManager::Initialize()
	{
		m_availableBindlessIndices.Allocate(g_rhiVulkanMaxBindlessDescriptors.GetValue());

		for (int32_t i = g_rhiVulkanMaxBindlessDescriptors.GetValue() - 1; i >= 0; --i)
		{
			m_availableBindlessIndices.Push(BindlessIndex(static_cast<uint32_t>(i)));
		}
	}

	BindlessIndex VulkanBindlessDescriptorManager::AllocateIndex()
	{
		BindlessIndex result;
		m_availableBindlessIndices.Pop(result);

		return result;
	}

	void VulkanBindlessDescriptorManager::FreeIndex(BindlessIndex index)
	{
		m_availableBindlessIndices.Push(index);
	}
}
