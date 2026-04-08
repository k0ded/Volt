#pragma once

#include <RHIModule/RayTracing/ShaderBindingTable.h>
#include <RHIModule/Buffers/Buffer.h>

namespace Volt::RHI
{
	class Buffer;
	class VulkanShaderBindingTable final : public ShaderBindingTable
	{
	public:
		VulkanShaderBindingTable(IntRef<RayTracingPipeline> pipeline);
		~VulkanShaderBindingTable() override;

		void Invalidate() override;
		bool IsShaderInTable(IntRef<Shader> shader) const override;

		VT_NODISCARD VT_INLINE IntRef<Buffer> GetRayGenTable() const { return m_rayGenBindingTable; }
		VT_NODISCARD VT_INLINE IntRef<Buffer> GetMissTable() const { return m_missBindingTable; }
		VT_NODISCARD VT_INLINE IntRef<Buffer> GetHitGroupTable() const { return m_hitGroupBindingTable; }
		VT_NODISCARD VT_INLINE IntRef<Buffer> GetCallableTable() const { return m_callableBindingTable; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();

		IntRef<RayTracingPipeline> m_pipeline;
		
		IntRef<Buffer> m_rayGenBindingTable;
		IntRef<Buffer> m_missBindingTable;
		IntRef<Buffer> m_hitGroupBindingTable;
		IntRef<Buffer> m_callableBindingTable;
	};
}
