#pragma once

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader2.h>

struct VkDescriptorSetLayout_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanComputePipeline2 : public ComputePipeline
	{
	public:
		VulkanComputePipeline2(RefPtr<Shader2> shader);
		~VulkanComputePipeline2() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		bool IsValid() const override;
		size_t GetHash() const override;

		VT_NODISCARD VT_INLINE const Vector<std::pair<uint32_t, uint32_t>>& GetDescriptorPoolSizes() const { return m_descriptorPoolSizes; }
		VT_NODISCARD VT_INLINE const Vector<VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE const ShaderBindings& GetBindings() const { return m_pipelineBindings; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		RefPtr<Shader2> m_shader;
		size_t m_hash;

		Vector<VkDescriptorSetLayout_T*> m_descriptorSetLayouts;
		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		ShaderBindings m_pipelineBindings;
		Vector<std::pair<uint32_t, uint32_t>> m_descriptorPoolSizes;
	};
}
