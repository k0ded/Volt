#include "vkpch.h"

#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"
#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"

#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Buffers/CommandBufferUtility.h>

namespace Volt::RHI
{
	VulkanShaderBindingTable::VulkanShaderBindingTable(IntRef<RayTracingPipeline> pipeline)
		: m_pipeline(pipeline)
	{
		Invalidate();
	}
	
	VulkanShaderBindingTable::~VulkanShaderBindingTable()
	{
		Release();
	}

	bool VulkanShaderBindingTable::IsShaderInTable(IntRef<Shader> shader) const
	{
		return m_pipeline->IsShaderInPipeline(shader);
	}
	
	void* VulkanShaderBindingTable::GetHandleImpl() const
	{
		return nullptr;
	}
	
	void VulkanShaderBindingTable::Release()
	{
		m_rayGenBindingTable = nullptr;
		m_missBindingTable = nullptr;
		m_hitGroupBindingTable = nullptr;
		m_callableBindingTable = nullptr;
	}

	void VulkanShaderBindingTable::Invalidate()
	{
		Release();

		VulkanRayTracingPipeline& vulkanPipeline = m_pipeline->AsRef<VulkanRayTracingPipeline>();

		IntRef<CommandBuffer> commandBuffer = CommandBuffer::Create();
		commandBuffer->Begin();

		BarrierVector barriers{};

		// RayGen
		{
			const auto& rayGenData = vulkanPipeline.GetRayGenData();

			if (!rayGenData.shaderHandles.empty())
			{
				BufferDesc desc{};
				desc.numElements = static_cast<uint32_t>(rayGenData.shaderHandles.size());
				desc.elementSize = sizeof(uint8_t);
				desc.debugName = "RayGen SBT";
				desc.usage = BufferUsage::StorageBuffer | BufferUsage::ShaderBindingTable | BufferUsage::DeviceAddress;
				desc.memoryUsage = MemoryUsage::GPU;

				m_rayGenBindingTable = Buffer::Create(desc);
				//m_rayGenBindingTable->SetData(commandBuffer, rayGenData.shaderHandles.data(), rayGenData.shaderHandles.size() * sizeof(uint8_t));

				auto& barrier = barriers.emplace_back();
				barrier.type = BarrierType::Buffer;
				barrier.bufferBarrier().srcAccess = BarrierAccess::None;
				barrier.bufferBarrier().srcStage = BarrierStage::All;
				barrier.bufferBarrier().dstStage = BarrierStage::RayTracingShader;
				barrier.bufferBarrier().dstAccess = BarrierAccess::ShaderBindingTableRead;
				barrier.bufferBarrier().offset = 0;
				barrier.bufferBarrier().size = m_rayGenBindingTable->GetMemoryRequirements().size;
				barrier.bufferBarrier().resource = m_rayGenBindingTable;
			}
		}

		// Miss
		{
			const auto& missData = vulkanPipeline.GetMissData();

			if (!missData.shaderHandles.empty())
			{
				BufferDesc desc{};
				desc.numElements = static_cast<uint32_t>(missData.shaderHandles.size());
				desc.elementSize = sizeof(uint8_t);
				desc.debugName = "Miss SBT";
				desc.usage = BufferUsage::StorageBuffer | BufferUsage::ShaderBindingTable | BufferUsage::DeviceAddress;
				desc.memoryUsage = MemoryUsage::GPU;

				m_missBindingTable = Buffer::Create(desc);
				//m_missBindingTable->SetData(commandBuffer, missData.shaderHandles.data(), missData.shaderHandles.size() * sizeof(uint8_t));

				auto& barrier = barriers.emplace_back();
				barrier.type = BarrierType::Buffer;
				barrier.bufferBarrier().srcAccess = BarrierAccess::None;
				barrier.bufferBarrier().srcStage = BarrierStage::All;
				barrier.bufferBarrier().dstStage = BarrierStage::RayTracingShader;
				barrier.bufferBarrier().dstAccess = BarrierAccess::ShaderBindingTableRead;
				barrier.bufferBarrier().offset = 0;
				barrier.bufferBarrier().size = m_missBindingTable->GetMemoryRequirements().size;
				barrier.bufferBarrier().resource = m_missBindingTable;
			}
		}

		// Hit group
		{
			const auto& hitGroupData = vulkanPipeline.GetHitGroupData();

			if (!hitGroupData.shaderHandles.empty())
			{
				BufferDesc desc{};
				desc.numElements = static_cast<uint32_t>(hitGroupData.shaderHandles.size());
				desc.elementSize = sizeof(uint8_t);
				desc.debugName = "Hit SBT";
				desc.usage = BufferUsage::StorageBuffer | BufferUsage::ShaderBindingTable | BufferUsage::DeviceAddress;
				desc.memoryUsage = MemoryUsage::GPU;

				m_hitGroupBindingTable = Buffer::Create(desc);
				//m_hitGroupBindingTable->SetData(commandBuffer, hitGroupData.shaderHandles.data(), hitGroupData.shaderHandles.size() * sizeof(uint8_t));

				auto& barrier = barriers.emplace_back();
				barrier.type = BarrierType::Buffer;
				barrier.bufferBarrier().srcAccess = BarrierAccess::None;
				barrier.bufferBarrier().srcStage = BarrierStage::All;
				barrier.bufferBarrier().dstStage = BarrierStage::RayTracingShader;
				barrier.bufferBarrier().dstAccess = BarrierAccess::ShaderBindingTableRead;
				barrier.bufferBarrier().offset = 0;
				barrier.bufferBarrier().size = m_hitGroupBindingTable->GetMemoryRequirements().size;
				barrier.bufferBarrier().resource = m_hitGroupBindingTable;
			}
		}

		// Callable
		{
			const auto& callableData = vulkanPipeline.GetCallableData();

			if (!callableData.shaderHandles.empty())
			{
				BufferDesc desc{};
				desc.numElements = static_cast<uint32_t>(callableData.shaderHandles.size());
				desc.elementSize = sizeof(uint8_t);
				desc.debugName = "Callable SBT";
				desc.usage = BufferUsage::StorageBuffer | BufferUsage::ShaderBindingTable | BufferUsage::DeviceAddress;
				desc.memoryUsage = MemoryUsage::GPU;

				m_callableBindingTable = Buffer::Create(desc);
				//m_callableBindingTable->SetData(commandBuffer, callableData.shaderHandles.data(), callableData.shaderHandles.size() * sizeof(uint8_t));

				auto& barrier = barriers.emplace_back();
				barrier.type = BarrierType::Buffer;
				barrier.bufferBarrier().srcAccess = BarrierAccess::None;
				barrier.bufferBarrier().srcStage = BarrierStage::All;
				barrier.bufferBarrier().dstStage = BarrierStage::RayTracingShader;
				barrier.bufferBarrier().dstAccess = BarrierAccess::ShaderBindingTableRead;
				barrier.bufferBarrier().offset = 0;
				barrier.bufferBarrier().size = m_callableBindingTable->GetMemoryRequirements().size;
				barrier.bufferBarrier().resource = m_callableBindingTable;
			}
		}

		commandBuffer->ResourceBarrier(barriers);
		commandBuffer->End();

		CommandBufferUtils::ExecuteCommandBufferWithNewFence(commandBuffer);
	}
}
