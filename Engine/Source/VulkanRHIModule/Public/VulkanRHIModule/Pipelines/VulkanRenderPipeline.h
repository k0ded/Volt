#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/Pipelines/RenderPipeline.h>

struct VkPipelineLayout_T;
struct VkPipeline_T;

namespace Volt::RHI
{
	class VulkanRenderPipeline final : public RenderPipeline
	{
	public:
		VulkanRenderPipeline(const RenderPipelineCreateInfo& createInfo);
		~VulkanRenderPipeline() override;
		
		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name, ShaderStage shaderStage) const override;
		const Vector<ShaderParameterMap>& GetShaderParameterMaps() const override { static Vector<ShaderParameterMap> s; return s; }

		inline VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		RenderPipelineCreateInfo m_createInfo{};

		VkPipelineLayout_T* m_pipelineLayout = nullptr;
		VkPipeline_T* m_pipeline = nullptr;

		size_t m_hash = 0;
	};
}
