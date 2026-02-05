#pragma once

#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"

#include <RHIModule/Pipelines/RenderPipeline.h>

#include <CoreUtilities/Containers/VectorVariants.h>

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
		bool HasInlineParameters() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const override;
		ArrayView<ShaderParameterMap> GetShaderParameterMaps() const override;
		const PipelineShadersVector& GetShaders() const override;
		const VertexBufferLayout& GetVertexBufferLayout() const override;

		VT_NODISCARD VT_INLINE const Map<uint32_t, VkDescriptorSetLayout_T*>& GetDescriptorSetLayouts() const { return m_descriptorSets.descriptorSetLayouts; }
		VT_NODISCARD VT_INLINE const DescriptorSetLayoutBuilder::DescriptorSets& GetDescriptorSets() const { return m_descriptorSets; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();
		void VerifyShaderStages();

		RenderPipelineCreateInfo m_createInfo{};
		size_t m_hash;

		DescriptorSetLayoutBuilder::DescriptorSets m_descriptorSets;

		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;

		Array<ShaderParameterMap, GetNumShaderStages()> m_shaderParameterMaps;
		VertexBufferLayout m_vertexBufferLayout;
	
		bool m_hasPushConstants = false;
	};
}
