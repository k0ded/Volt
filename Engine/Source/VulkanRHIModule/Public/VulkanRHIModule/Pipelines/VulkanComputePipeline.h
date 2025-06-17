#pragma once

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>

struct VkDescriptorSetLayout_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanComputePipeline : public ComputePipeline
	{
	public:
		VulkanComputePipeline(RefPtr<Shader> shader);
		~VulkanComputePipeline() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader2() const override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name) const override;
		const ShaderParameterMap& GetShaderParameterMap() const override;

		VT_NODISCARD VT_INLINE const Vector<std::pair<uint32_t, uint32_t>>& GetDescriptorPoolSizes() const { return m_descriptorPoolSizes; }
		VT_NODISCARD VT_INLINE const Map<uint32_t, VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		RefPtr<Shader> m_shader;
		size_t m_hash;

		Map<uint32_t, VkDescriptorSetLayout_T*> m_descriptorSetLayouts;
		Vector<VkDescriptorSetLayout_T*> m_pipelineLayoutDescriptorSetLayouts;

		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		ShaderParameterMap m_shaderParameterMap;
		Vector<std::pair<uint32_t, uint32_t>> m_descriptorPoolSizes;
	};
}
