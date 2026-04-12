#pragma once

#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"
#include "VulkanRHIModule/LastSubmissionTracker.h"

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/Shader.h>

struct VkDescriptorSetLayout_T;
struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanComputePipeline final : public ComputePipeline, public LastSubmissionTracker
	{
	public:
		VulkanComputePipeline(IntRef<Shader> shader);
		~VulkanComputePipeline() override;

		void Invalidate() override;
		IntRef<Shader> GetShader() const override;
		bool IsValid() const override;
		bool HasInlineParameters() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name) const override;
		const ShaderParameterMap& GetShaderParameterMap() const override;

		VT_NODISCARD VT_INLINE const Vector<std::pair<uint32_t, uint32_t>>& GetDescriptorPoolSizes() const { return m_descriptorPoolSizes; }
		VT_NODISCARD VT_INLINE const Map<uint32_t, VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSets.descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE const DescriptorSetLayoutBuilder::DescriptorSets& GetDescriptorSets() const { return m_descriptorSets; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		IntRef<Shader> m_shader;
		size_t m_hash;

		DescriptorSetLayoutBuilder::DescriptorSets m_descriptorSets;

		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		ShaderParameterMap m_shaderParameterMap;
		Vector<std::pair<uint32_t, uint32_t>> m_descriptorPoolSizes;

		bool m_hasPushConstants = false;
	};
}
