#pragma once

#include <RHIModule/Descriptors/BindlessIndex.h>
#include <RHIModule/Memory/Allocation.h>

#include <CoreUtilities/Containers/AtomicStack.h>
#include <CoreUtilities/Allocators/Handle.h>

struct VkDescriptorSetLayout_T;
struct VkDescriptorGetInfoEXT;

namespace Volt::RHI
{
	class VulkanBindlessDescriptorManager
	{
	public:
		VulkanBindlessDescriptorManager();
		~VulkanBindlessDescriptorManager();

		BindlessIndex AllocateIndex();
		BindlessIndex AllocateSamplerIndex();

		//Note: When the bindless index is freed, the index
		//		might be repurposed immediatley, meaning the index
		//		should not be used after freeing.
		void FreeIndex(BindlessIndex index);
		void FreeSamplerIndex(BindlessIndex index);

		void UpdateDescriptor(BindlessIndex index, const VkDescriptorGetInfoEXT& descriptorGetInfo, uint64_t descriptorSize);
		void UpdateSamplerDescriptor(BindlessIndex index, const VkDescriptorGetInfoEXT& descriptorGetInfo, uint64_t descriptorSize);

		VT_INLINE VkDescriptorSetLayout_T* GetDescriptorSetLayout() const { return m_bindlessDescriptorSetLayout; }
		VT_INLINE uint64_t GetDescriptorBufferAddress() const { return m_descriptorBuffer->GetDeviceAddress(); }

		static VulkanBindlessDescriptorManager& Get() { return *s_instance; }

	private:
		void Initialize();
		void Release();

		void CreateBindlessDescriptorSetLayout();
		void CreateDescriptorBuffer();

		inline static VulkanBindlessDescriptorManager* s_instance = nullptr;

		AtomicStack<BindlessIndex> m_availableResourceBindlessIndices;
		AtomicStack<BindlessIndex> m_availableSamplerBindlessIndices;

		VkDescriptorSetLayout_T* m_bindlessDescriptorSetLayout = nullptr;

		Handle<Allocation> m_descriptorBuffer;

		uint64_t m_descriptorLayoutSize = 0;
		uint64_t m_descriptorSize = 0;

		uint64_t m_resourceDescriptorOffset = 0;
		uint64_t m_samplerDescriptorOffset = 0;

		uint8_t* m_descriptorBufferPtr = nullptr;
	};
}
