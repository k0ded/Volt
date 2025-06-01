#pragma once

#include <RHIModule/Pipelines/RayTracingPipeline.h>

struct VkPipelineLayout_T;
struct VkPipeline_T;

namespace Volt::RHI
{
	class VulkanRayTracingPipeline final : public RayTracingPipeline
	{
	public:
		struct RayTracingShaderData
		{
			Vector<RefPtr<Shader>> shaders;
			Vector<uint8_t> shaderHandles;

			inline void Clear()
			{
				shaders.clear();
				shaderHandles.clear();
			}
		};

		VulkanRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo);
		~VulkanRayTracingPipeline() override;

		void Invalidate() override;
		bool IsValid() const override;
		bool IsShaderInPipeline(RefPtr<Shader> shader) const override;
		const ShaderUniforms& GetRenderGraphConstants() const override;

		VT_NODISCARD VT_INLINE const RayTracingShaderData& GetRayGenData() const { return m_rayGenData; }
		VT_NODISCARD VT_INLINE const RayTracingShaderData& GetMissData() const { return m_missData; }
		VT_NODISCARD VT_INLINE const RayTracingShaderData& GetHitGroupData() const { return m_hitGroupData; }
		VT_NODISCARD VT_INLINE const RayTracingShaderData& GetCallableData() const { return m_callableData; }
		VT_NODISCARD VT_INLINE VkPipelineLayout_T* GetPipelineLayout() const { return m_pipelineLayout; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();

		RayTracingPipelineCreateInfo m_createInfo{};
	
		VkPipelineLayout_T* m_pipelineLayout = nullptr;
		VkPipeline_T* m_pipeline = nullptr;

		RayTracingShaderData m_rayGenData;
		RayTracingShaderData m_missData;
		RayTracingShaderData m_hitGroupData;
		RayTracingShaderData m_callableData;
	};
}
