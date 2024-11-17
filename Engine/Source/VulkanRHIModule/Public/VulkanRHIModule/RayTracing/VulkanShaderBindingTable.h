#pragma once

#include <RHIModule/RayTracing/ShaderBindingTable.h>

namespace Volt::RHI
{
	class StorageBuffer;
	class VulkanShaderBindingTable final : public ShaderBindingTable
	{
	public:
		VulkanShaderBindingTable(RefPtr<RayTracingPipeline> pipeline);
		~VulkanShaderBindingTable() override;

		VT_NODISCARD VT_INLINE RefPtr<StorageBuffer> GetRayGenTable() const { return m_rayGenBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<StorageBuffer> GetMissTable() const { return m_missBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<StorageBuffer> GetHitGroupTable() const { return m_hitGroupBindingTable; }
		VT_NODISCARD VT_INLINE RefPtr<StorageBuffer> GetCallableTable() const { return m_callableBindingTable; }

	protected:
		void* GetHandleImpl() const override;

	private:
		void Release();
		void Invalidate();

		RefPtr<RayTracingPipeline> m_pipeline;
		
		RefPtr<StorageBuffer> m_rayGenBindingTable;
		RefPtr<StorageBuffer> m_missBindingTable;
		RefPtr<StorageBuffer> m_hitGroupBindingTable;
		RefPtr<StorageBuffer> m_callableBindingTable;
	};
}
