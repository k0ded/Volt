#pragma once

#include <RHIModule/Pipelines/RenderPipeline.h>

struct VkDescriptorSetLayout_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanRenderPipeline : public RenderPipeline
	{
	public:
		VulkanRenderPipeline(const RenderPipelineCreateInfo& createInfo);
		~VulkanRenderPipeline() override;

		void Invalidate() override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const override;
		const Vector<ShaderParameterMap>& GetShaderParameterMaps() const override;

		VT_NODISCARD VT_INLINE const vt::map<uint32_t, VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE const Vector<std::pair<uint32_t, uint32_t>>& GetDescriptorPoolSizes() const { return m_descriptorPoolSizes; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();
		void VerifyShaderStages();

		RenderPipelineCreateInfo m_createInfo{};
		size_t m_hash;

		vt::map<uint32_t, VkDescriptorSetLayout_T*> m_descriptorSetLayouts;
		Vector<VkDescriptorSetLayout_T*> m_pipelineLayoutDescriptorSetLayouts;

		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		Vector<ShaderParameterMap> m_shaderParameterMaps;
		Vector<std::pair<uint32_t, uint32_t>> m_descriptorPoolSizes;
	};
}
