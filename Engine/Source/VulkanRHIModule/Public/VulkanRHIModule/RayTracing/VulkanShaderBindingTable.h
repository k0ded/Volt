#pragma once

#include <RHIModule/RayTracing/ShaderBindingTable.h>

namespace Volt::RHI
{
	class Buffer;
	class VulkanShaderBindingTable final : public ShaderBindingTable
	{
	public:
		VulkanShaderBindingTable(RefPtr<RayTracingPipeline> pipeline);
		~VulkanShaderBindingTable() override;

		void Invalidate() override;
		bool IsShaderInTable(RefPtr<Shader> shader) const override;

		VT_NODISCARD VT_INLINE RefPtr<Buffer> GetRayGenTable() const { return m_rayGenBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<Buffer> GetMissTable() const { return m_missBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<Buffer> GetHitGroupTable() const { return m_hitGroupBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<Buffer> GetCallableTable() const { return m_callableBindingTable; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();

		RefPtr<RayTracingPipeline> m_pipeline;
		
		RefPtr<Buffer> m_rayGenBindingTable;
		RefPtr<Buffer> m_missBindingTable;
		RefPtr<Buffer> m_hitGroupBindingTable;
		RefPtr<Buffer> m_callableBindingTable;
	};
}
