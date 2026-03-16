#pragma once

#include <RHIModule/Shader/ShaderCommon.h>

#include <CoreUtilities/CompilerTraits.h>
#include <CoreUtilities/Containers/ArrayView.h>

struct VkDescriptorSetLayout_T;

namespace Volt::RHI
{
	class ResourceTableDescriptorSetManager
	{
	public:
		inline static constexpr uint32_t MaxSize = 8192;
		inline static constexpr uint32_t Set = GetNumBindableShaderStages();
		inline static constexpr uint32_t TexturesBinding = 0;
		inline static constexpr uint32_t BuffersBinding = 1;
		inline static constexpr uint32_t DescriptorTypeCount = 2;

		ResourceTableDescriptorSetManager();
		~ResourceTableDescriptorSetManager();

		VT_INLINE static ResourceTableDescriptorSetManager& Get() { return *s_instance; }
		VT_INLINE VkDescriptorSetLayout_T* GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
		VT_INLINE uint64_t GetDescriptorSetLayoutSize() const { return m_descriptorSetLayoutSize; }
		VT_INLINE ArrayView<uint64_t> GetBindingOffsets() const { return m_bindingOffsets; }

	private:
		inline static ResourceTableDescriptorSetManager* s_instance;
	
		void CreateDescriptorSetLayout();

		VkDescriptorSetLayout_T* m_descriptorSetLayout = nullptr;
		Array<uint64_t, DescriptorTypeCount> m_bindingOffsets;
		uint64_t m_descriptorSetLayoutSize = 0;
	};
}
