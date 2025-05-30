#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Shader/ShaderParameterMap.h>

struct VkPipeline_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class VulkanComputePipeline : public ComputePipeline
	{ 
	public:
		VulkanComputePipeline(RefPtr<Shader> shader, bool useGlobalResources);
		~VulkanComputePipeline() override;

		void Invalidate() override;
		RefPtr<Shader> GetShader() const override;
		RefPtr<Shader2> GetShader2() const override;
		bool IsValid() const override;
		size_t GetHash() const override;
		const ShaderResourceBinding* GetResourceBindingFromName(const StringHash& name) const override;
		const ShaderParameterMap& GetShaderParameterMap() const override { static ShaderParameterMap s; return s; }

		inline VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void GenerateHash();

		RefPtr<Shader> m_shader;
		bool m_useGlobalResouces = false;

		VkPipeline_T* m_pipeline = nullptr;
		VkPipelineLayout_T* m_pipelineLayout = nullptr;
	
		size_t m_hash = 0;
	};
}
