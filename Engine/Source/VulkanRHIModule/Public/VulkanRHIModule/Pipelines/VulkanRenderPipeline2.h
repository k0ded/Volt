#pragma once

#include <RHIModule/Pipelines/RenderPipeline.h>

struct VkDescriptorSetLayout_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanRenderPipeline2 : public RenderPipeline
	{
	public:
		VulkanRenderPipeline2(const RenderPipelineCreateInfo& createInfo);
		~VulkanRenderPipeline2() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		bool IsValid() const override;
		size_t GetHash() const override;

		VT_NODISCARD VT_INLINE const Vector<VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE const Vector<std::pair<uint32_t, uint32_t>>& GetDescriptorPoolSizes() const { return m_descriptorPoolSizes; }
		VT_NODISCARD VT_INLINE const ShaderBindings& GetBindings() const { return m_pipelineBindings; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();
		void VerifyShaderStages();

		RenderPipelineCreateInfo m_createInfo{};
		size_t m_hash;

		Vector<VkDescriptorSetLayout_T*> m_descriptorSetLayouts;
		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		ShaderBindings m_pipelineBindings;
		Vector<std::pair<uint32_t, uint32_t>> m_descriptorPoolSizes;
	};
}
