#pragma once

#include <RHIModule/Descriptors/BindlessIndex.h>

#include <CoreUtilities/Containers/AtomicStack.h>

namespace Volt::RHI
{
	class VulkanBindlessDescriptorManager
	{
	public:
		VulkanBindlessDescriptorManager();
		~VulkanBindlessDescriptorManager();

		BindlessIndex AllocateIndex();

		//Note: When the bindless index is freed, the index
		//		might be repurposed immediatley, meaning the index
		//		should not be used after freeing.
		void FreeIndex(BindlessIndex index);

		static VulkanBindlessDescriptorManager& Get() { return *s_instance; }

	private:
		void Initialize();

		inline static VulkanBindlessDescriptorManager* s_instance = nullptr;

		AtomicStack<BindlessIndex> m_availableBindlessIndices;
	};
}
