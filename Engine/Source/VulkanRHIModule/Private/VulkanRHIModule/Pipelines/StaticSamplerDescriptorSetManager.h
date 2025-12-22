#pragma once

#include <CoreUtilities/CompilerTraits.h>

struct VkDescriptorSetLayout_T;
struct VkSampler_T;

namespace Volt::RHI
{
	class StaticSamplerDescriptorSetManager
	{
	public:
		inline static constexpr uint32_t Set = 10;

		StaticSamplerDescriptorSetManager();
		~StaticSamplerDescriptorSetManager();
	
		VT_NODISCARD VT_INLINE VkDescriptorSetLayout_T* GetDescriptorSetLayout() const { return m_descriptorSetLayout; }

		VT_INLINE static StaticSamplerDescriptorSetManager& Get() { return *s_instance; }

	private:
		inline static StaticSamplerDescriptorSetManager* s_instance;

		void CreateDescriptorSetLayout();
		void CreateSamplerStates();

		VkDescriptorSetLayout_T* m_descriptorSetLayout = nullptr;
		Vector<VkSampler_T*> m_staticSamplers;
	};
}
