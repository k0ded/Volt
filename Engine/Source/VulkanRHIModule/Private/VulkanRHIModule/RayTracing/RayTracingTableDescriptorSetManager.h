#pragma once

#include <CoreUtilities/CompilerTraits.h>

struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class RayTracingTableDescriptorSetManager
	{
	public:
		inline static constexpr uint32_t MaxSize = 8192;
		inline static constexpr uint32_t Set = 9;
		inline static constexpr uint32_t TexturesBinding = 0;
		inline static constexpr uint32_t BuffersBinding = 1;

		RayTracingTableDescriptorSetManager();
		~RayTracingTableDescriptorSetManager();

		VT_INLINE static RayTracingTableDescriptorSetManager& Get() { return *s_instance; }
		VT_INLINE VkDescriptorSetLayout_T* GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

	private:
		void CreateDescriptorSetLayout();

		inline static RayTracingTableDescriptorSetManager* s_instance;
	
		VkDescriptorSetLayout_T* m_descriptorSetLayout = nullptr;
	};
}
