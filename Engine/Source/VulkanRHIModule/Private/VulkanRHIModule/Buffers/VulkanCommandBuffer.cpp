 #include "vkpch.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"

#include "VulkanRHIModule/Common/VulkanCommon.h"
#include "VulkanRHIModule/Common/VulkanHelpers.h"
#include "VulkanRHIModule/Common/VulkanFunctions.h"
#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"

#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorTable.h"
#include "VulkanRHIModule/Descriptors/VulkanDescriptorTable.h"

#include "VulkanRHIModule/Images/VulkanImage.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Synchronization/VulkanEvent.h"

#include "VulkanRHIModule/RayTracing/VulkanRayTracingHelpers.h"
#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>
#include <RHIModule/Graphics/DeviceQueue.h>

#include <RHIModule/Pipelines/ComputePipeline.h>
#include <RHIModule/Pipelines/RenderPipeline.h>

#include <RHIModule/Memory/Allocation.h>

#include <RHIModule/Images/ImageView.h>

#include <RHIModule/Core/Profiling.h>
#include <RHIModule/Core/RenderingInfo.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHIFeatures.h>

#include <RHIModule/RayTracing/AccelerationStructure.h>

#include <CoreUtilities/EnumUtils.h>

#ifdef VT_ENABLE_NV_AFTERMATH

#include <GFSDK_Aftermath.h>
#include <GFSDK_Aftermath_Defines.h>
#include <GFSDK_Aftermath_GpuCrashDump.h>

#include <RHIModule/Utility/NsightAftermathHelpers.h>

#endif

#include <CoreUtilities/MemoryUtility.h>

#include <vulkan/vulkan.h>

namespace Volt::RHI
{
	namespace Utility
	{
		const VkPipelineStageFlags2 GetStageFromBarrierStage(const BarrierStage barrierStage)
		{
			VkPipelineStageFlags2 result = VK_PIPELINE_STAGE_2_NONE;

#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
			if (EnumValueContainsFlag(barrierStage, BarrierStage::MeshShader) || EnumValueContainsFlag(barrierStage, BarrierStage::AmplificationShader))
			{
				VT_ENSURE(RHICanUseMeshShaders());
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::RayTracingShader))
			{
				VT_ENSURE(RHICanUseRayTracing());
			}
#endif

			if (EnumValueContainsFlag(barrierStage, BarrierStage::All))
			{
				result |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::IndexInput))
			{
				result |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VertexInput))
			{
				result |= VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VertexShader))
			{
				result |= VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::PixelShader))
			{
				result |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::DepthStencil))
			{
				result |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::RenderTarget))
			{
				result |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::ComputeShader))
			{
				result |= VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::RayTracingShader))
			{
				result |= VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::Copy))
			{
				result |= VK_PIPELINE_STAGE_2_COPY_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::Resolve))
			{
				result |= VK_PIPELINE_STAGE_2_RESOLVE_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::DrawIndirect))
			{
				result |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::AllGraphics))
			{
				result |= VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VideoDecode))
			{
				result |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VideoEncode))
			{
				result |= VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::MeshShader))
			{
				result |= VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::AmplificationShader))
			{
				result |= VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT;
			}

			return result;
		}

		const VkAccessFlags2 GetAccessFromBarrierAccess(const BarrierAccess barrierAccess)
		{
			VkAccessFlags2 result = VK_ACCESS_2_NONE;

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VertexBuffer))
			{
				result |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::UniformBuffer))
			{
				result |= VK_ACCESS_2_UNIFORM_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::IndexBuffer))
			{
				result |= VK_ACCESS_2_INDEX_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::RenderTarget))
			{
				result |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ShaderWrite))
			{
				result |= VK_ACCESS_2_SHADER_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::DepthStencilWrite))
			{
				result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::DepthStencilRead))
			{
				result |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ShaderRead))
			{
				result |= VK_ACCESS_2_SHADER_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::IndirectArgument))
			{
				result |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::CopyDest))
			{
				result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::CopySource))
			{
				result |= VK_ACCESS_2_TRANSFER_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ResolveDest))
			{
				result |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ResolveSource))
			{
				result |= VK_ACCESS_2_TRANSFER_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoEncodeRead))
			{
				result |= VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoEncodeWrite))
			{
				result |= VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoDecodeRead))
			{
				result |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoDecodeWrite))
			{
				result |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AllRead))
			{
				result |= VK_ACCESS_2_MEMORY_READ_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AllWrite))
			{
				result |= VK_ACCESS_2_MEMORY_WRITE_BIT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AccelerationStructureRead))
			{
				result |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AccelerationStructureWrite))
			{
				result |= VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ShaderBindingTableRead))
			{
				result |= VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR;
			}

			return result;
		}

		inline uint64_t CalculateStagingBufferSize(const ImageCopyData& copyData)
		{
			uint64_t size = 0;
			for (const auto& data : copyData.copySubData)
			{
				size += data.slicePitch;
			}

			return size;
		}
	}

	VulkanCommandBuffer::VulkanCommandBuffer(QueueType queueType)
		: m_queueType(queueType)
	{
		Invalidate();
	}

	VulkanCommandBuffer::VulkanCommandBuffer(const CommandBuffer* parentCommandBuffer)
		: m_queueType(parentCommandBuffer->GetQueueType()), m_commandBufferLevel(CommandBufferLevel::Secondary), m_parentCommandBuffer(parentCommandBuffer)
	{
		Invalidate();
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		Release();
	}

	void VulkanCommandBuffer::Begin(bool oneTimeSubmit)
	{
		VT_PROFILE_FUNCTION();

		auto device = GraphicsContext::GetDevice();

		VT_VK_CHECK(vkResetCommandPool(device->GetHandle<VkDevice>(), m_commandBufferData.commandPool, 0));

		// If the next timestamp query is zero, no frame has run before
		if (m_nextAvailableTimestampQuery > 0)
		{
			FetchTimestampResults();
		}

		// Begin command buffer
		{
			if (m_commandBufferLevel == CommandBufferLevel::Primary) BeginPrimaryInternal(oneTimeSubmit);
			else													 BeginSecondaryInternal(oneTimeSubmit);
		}

		if (m_hasTimestampSupport)
		{
			vkCmdResetQueryPool(m_commandBufferData.commandBuffer, m_timestampQueryPool, 0, m_timestampQueryCount);
			vkCmdWriteTimestamp2(m_commandBufferData.commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_timestampQueryPool, 0);

			m_timestampCount = m_nextAvailableTimestampQuery;
			m_nextAvailableTimestampQuery = 2;
		}
	}

	void VulkanCommandBuffer::End()
	{
		VT_PROFILE_FUNCTION();

		if (m_hasTimestampSupport)
		{
			vkCmdWriteTimestamp2(m_commandBufferData.commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_timestampQueryPool, 1);
		}

		VT_VK_CHECK(vkEndCommandBuffer(m_commandBufferData.commandBuffer));
	}

	void VulkanCommandBuffer::SetEvent(RawPtr<Event> event)
	{
		VT_PROFILE_FUNCTION();

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;
		depInfo.dependencyFlags = 0;
		//depInfo.

		//vkCmdSetEvent2()
	}

	void VulkanCommandBuffer::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDraw(m_commandBufferData.commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void VulkanCommandBuffer::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawIndexed(m_commandBufferData.commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void VulkanCommandBuffer::DrawIndexedIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawIndexedIndirect(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, drawCount, stride);
	}

	void VulkanCommandBuffer::DrawIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawIndirect(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, drawCount, stride);
	}

	void VulkanCommandBuffer::DrawIndexedIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawIndexedIndirectCount(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, countBuffer->GetHandle<VkBuffer>(), countBufferOffset, maxDrawCount, stride);
	}

	void VulkanCommandBuffer::DrawIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawIndirectCount(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, countBuffer->GetHandle<VkBuffer>(), countBufferOffset, maxDrawCount, stride);
	}

	void VulkanCommandBuffer::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentComputePipeline != nullptr);
#endif

		vkCmdDispatch(m_commandBufferData.commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanCommandBuffer::DispatchIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentComputePipeline != nullptr);
#endif

		vkCmdDispatchIndirect(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset);
	}

	void VulkanCommandBuffer::DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawMeshTasksEXT(m_commandBufferData.commandBuffer, groupCountX, groupCountY, groupCountZ);
	}

	void VulkanCommandBuffer::DispatchMeshTasksIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawMeshTasksIndirectEXT(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, drawCount, stride);
	}

	void VulkanCommandBuffer::DispatchMeshTasksIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRenderPipeline != nullptr);
#endif

		vkCmdDrawMeshTasksIndirectCountEXT(m_commandBufferData.commandBuffer, commandsBuffer->GetHandle<VkBuffer>(), offset, countBuffer->GetHandle<VkBuffer>(), countBufferOffset, maxDrawCount, stride);
	}

	void VulkanCommandBuffer::TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_currentRayTracingPipeline != nullptr);
#endif

		const auto& rayTracingPipelineProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetDeviceProperties().rayTracingPipelineProperties;

		auto GetStridedDeviceAddressRegion = [&rayTracingPipelineProperties](const VulkanRayTracingPipeline::RayTracingShaderData& data, RefPtr<StorageBuffer> bindingTable)
		{
			VkStridedDeviceAddressRegionKHR result
			{
				.deviceAddress = 0,
				.stride = 0,
				.size = 0
			};

			if (data.shaderHandles.empty() || !bindingTable)
			{
				return result;
			}

			result.deviceAddress = bindingTable->GetDeviceAddress();
			result.stride = rayTracingPipelineProperties.shaderGroupHandleSize;
			result.size = static_cast<uint32_t>(data.shaderHandles.size());

			return result;
		};

		VulkanRayTracingPipeline& vulkanPipeline = m_currentRayTracingPipeline->AsRef<VulkanRayTracingPipeline>();
		VulkanShaderBindingTable& vulkanSBT = shaderBindingTable->AsRef<VulkanShaderBindingTable>();

		VkStridedDeviceAddressRegionKHR rayGenTable = GetStridedDeviceAddressRegion(vulkanPipeline.GetRayGenData(), vulkanSBT.GetRayGenTable());
		VkStridedDeviceAddressRegionKHR missTable = GetStridedDeviceAddressRegion(vulkanPipeline.GetMissData(), vulkanSBT.GetMissTable());
		VkStridedDeviceAddressRegionKHR hitGroupTable = GetStridedDeviceAddressRegion(vulkanPipeline.GetHitGroupData(), vulkanSBT.GetHitGroupTable());
		VkStridedDeviceAddressRegionKHR callableTable = GetStridedDeviceAddressRegion(vulkanPipeline.GetCallableData(), vulkanSBT.GetCallableTable());

		vkCmdTraceRaysKHR(m_commandBufferData.commandBuffer, &rayGenTable, &missTable, &hitGroupTable, &callableTable, width, height, depth);
	}

	void VulkanCommandBuffer::SetViewports(const StackVector<Viewport, MAX_VIEWPORT_COUNT>& viewports)
	{
		vkCmdSetViewport(m_commandBufferData.commandBuffer, 0, static_cast<uint32_t>(viewports.Size()), reinterpret_cast<const VkViewport*>(viewports.Data()));
	}

	void VulkanCommandBuffer::SetScissors(const StackVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors)
	{
		vkCmdSetScissor(m_commandBufferData.commandBuffer, 0, static_cast<uint32_t>(scissors.Size()), reinterpret_cast<const VkRect2D*>(scissors.Data()));
	}

	void VulkanCommandBuffer::BindPipeline(RawPtr<RenderPipeline> pipeline)
	{
		VT_ENSURE(pipeline);

		ClearCurrentPipeline();

		m_currentRenderPipeline = pipeline;
		vkCmdBindPipeline(m_commandBufferData.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle<VkPipeline>());
	}

	void VulkanCommandBuffer::BindPipeline(RawPtr<ComputePipeline> pipeline)
	{
		VT_ENSURE(pipeline);

		ClearCurrentPipeline();

		m_currentComputePipeline = pipeline;
		vkCmdBindPipeline(m_commandBufferData.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle<VkPipeline>());
	}

	void VulkanCommandBuffer::BindPipeline(RawPtr<RayTracingPipeline> pipeline)
	{
		VT_ENSURE(pipeline);

		ClearCurrentPipeline();

		m_currentRayTracingPipeline = pipeline;
		vkCmdBindPipeline(m_commandBufferData.commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline->GetHandle<VkPipeline>());
	}

	void VulkanCommandBuffer::BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding)
	{
		Vector<VkBuffer, InlineAllocator<MAX_VERTEX_BUFFER_COUNT>> vkBuffers;
		Vector<VkDeviceSize, InlineAllocator<MAX_VERTEX_BUFFER_COUNT>> offsets;

		for (size_t i = 0; i < vertexBuffers.size(); i++)
		{
			vkBuffers.emplace_back() = vertexBuffers[i]->GetHandle<VkBuffer>();
			offsets.emplace_back(0u);
		}

		vkCmdBindVertexBuffers(m_commandBufferData.commandBuffer, firstBinding, static_cast<uint32_t>(vkBuffers.size()), vkBuffers.data(), offsets.data());
	}

	void VulkanCommandBuffer::BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer, const IndexType indexType)
	{
		constexpr VkDeviceSize offset = 0;
		vkCmdBindIndexBuffer(m_commandBufferData.commandBuffer, indexBuffer->GetHandle<VkBuffer>(), offset, indexType == IndexType::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
	}

	void VulkanCommandBuffer::BindDescriptorTable(RawPtr<DescriptorTable> descriptorTable)
	{
		VulkanDescriptorTable& vulkanTable = descriptorTable->AsRef<VulkanDescriptorTable>();
		vulkanTable.PrepareForRender();

		const VkPipelineBindPoint bindPoint = static_cast<VkPipelineBindPoint>(vulkanTable.GetRelatedBindPoint());
		const Map<uint32_t, VkDescriptorSet>& descriptorSets = vulkanTable.GetDescriptorSets();
		VkPipelineLayout pipelineLayout = vulkanTable.GetRelatedPipelineLayout();

		for (const auto& [setIndex, descriptorSet] : descriptorSets)
		{
			vkCmdBindDescriptorSets(m_commandBufferData.commandBuffer, bindPoint, pipelineLayout, setIndex, 1, &descriptorSet, 0, nullptr);
		}
	}

	void VulkanCommandBuffer::BindDescriptorTable(RawPtr<BindlessDescriptorTable> descriptorTable, RawPtr<UniformBuffer> constantsBuffer, const uint32_t offsetIndex, const uint32_t stride, RawPtr<AccelerationStructure> accelerationStructure)
	{
		descriptorTable->AsRef<VulkanBindlessDescriptorTable>().Bind(*this, constantsBuffer, offsetIndex, stride, accelerationStructure);
	}

	void VulkanCommandBuffer::BeginRendering(const RenderingInfo& renderingInfo)
	{
		StackVector<VkRenderingAttachmentInfo, MAX_COLOR_ATTACHMENT_COUNT> colorAttachmentInfo{};
		VkRenderingAttachmentInfo depthAttachmentInfo{};

		for (const auto& colorAtt : renderingInfo.colorAttachments)
		{
			VkRenderingAttachmentInfo& newInfo = colorAttachmentInfo.EmplaceBack();
			newInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			newInfo.imageView = colorAtt.view->GetHandle<VkImageView>();
			newInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			newInfo.loadOp = Utility::VoltToVulkanLoadOp(colorAtt.clearMode);
			newInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			memcpy_s(&newInfo.clearValue, sizeof(uint32_t) * 4, &colorAtt.clearColor, sizeof(uint32_t) * 4);
		}

		const bool hasDepth = renderingInfo.depthAttachmentInfo.view;

		if (hasDepth)
		{
			depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachmentInfo.imageView = renderingInfo.depthAttachmentInfo.view->GetHandle<VkImageView>();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			depthAttachmentInfo.loadOp = Utility::VoltToVulkanLoadOp(renderingInfo.depthAttachmentInfo.clearMode);
			depthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			memcpy_s(&depthAttachmentInfo.clearValue.depthStencil, sizeof(uint32_t) * 2, &renderingInfo.depthAttachmentInfo.clearColor, sizeof(uint32_t) * 2);
		}

		VkRenderingInfo vkRenderingInfo{};
		vkRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		vkRenderingInfo.renderArea = { renderingInfo.renderArea.offset.x, renderingInfo.renderArea.offset.y, renderingInfo.renderArea.extent.width, renderingInfo.renderArea.extent.height };
		vkRenderingInfo.layerCount = renderingInfo.layerCount;
		vkRenderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentInfo.Size());
		vkRenderingInfo.pColorAttachments = colorAttachmentInfo.Data();
		vkRenderingInfo.pStencilAttachment = nullptr;

		if (hasDepth)
		{
			vkRenderingInfo.pDepthAttachment = &depthAttachmentInfo;
		}
		else
		{
			vkRenderingInfo.pDepthAttachment = nullptr;
		}

		vkCmdBeginRendering(m_commandBufferData.commandBuffer, &vkRenderingInfo);
	}

	void VulkanCommandBuffer::EndRendering()
	{
		vkCmdEndRendering(m_commandBufferData.commandBuffer);
	}

	void AddGlobalBarrier(const GlobalBarrier& barrierInfo, VkMemoryBarrier2& outBarrier)
	{
		outBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
		outBarrier.pNext = nullptr;
		outBarrier.srcAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.srcAccess);
		outBarrier.dstAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.dstAccess);
		outBarrier.srcStageMask = Utility::GetStageFromBarrierStage(barrierInfo.srcStage);
		outBarrier.dstStageMask = Utility::GetStageFromBarrierStage(barrierInfo.dstStage);
	}

	void AddBufferBarrier(const BufferBarrier& barrierInfo, VkBufferMemoryBarrier2& outBarrier)
	{
		VT_ENSURE(barrierInfo.resource != nullptr);
		auto& vkBuffer = barrierInfo.resource->AsRef<VulkanStorageBuffer>();

		outBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
		outBarrier.pNext = nullptr;

		if (barrierInfo.srcStage == BarrierStage::Clear)
		{
			outBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			outBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		else
		{
			outBarrier.srcAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.srcAccess);
			outBarrier.srcStageMask = Utility::GetStageFromBarrierStage(barrierInfo.srcStage);
		}

		if (barrierInfo.dstStage == BarrierStage::Clear)
		{
			outBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			outBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		else
		{
			outBarrier.dstAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.dstAccess);
			outBarrier.dstStageMask = Utility::GetStageFromBarrierStage(barrierInfo.dstStage);
		}

		outBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		outBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		outBarrier.offset = barrierInfo.offset;
		outBarrier.size = barrierInfo.size;
		outBarrier.buffer = vkBuffer.GetHandle<VkBuffer>();

		GraphicsContext::GetResourceStateTracker()->TransitionResource(barrierInfo.resource, barrierInfo.dstStage, barrierInfo.dstAccess);
	}

	void AddImageBarrier(const ImageBarrier& barrierInfo, VkImageMemoryBarrier2& outBarrier)
	{
		VT_ENSURE(barrierInfo.resource != nullptr);

		VkImageAspectFlags aspectFlags = Utility::GetVkImageAspect(barrierInfo.resource->As<Image>()->GetImageAspect());
		VT_ASSERT(aspectFlags != VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM && aspectFlags != VK_IMAGE_ASPECT_NONE);

		outBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		outBarrier.pNext = nullptr;

		if (barrierInfo.srcStage == BarrierStage::Clear)
		{
			outBarrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			outBarrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			outBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
		else
		{
			outBarrier.srcAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.srcAccess);
			outBarrier.srcStageMask = Utility::GetStageFromBarrierStage(barrierInfo.srcStage);
			outBarrier.oldLayout = Utility::GetVkImageLayoutFromImageLayout(barrierInfo.srcLayout);
		}

		if (barrierInfo.dstStage == BarrierStage::Clear)
		{
			outBarrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			outBarrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			outBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		}
		else
		{
			outBarrier.dstAccessMask = Utility::GetAccessFromBarrierAccess(barrierInfo.dstAccess);
			outBarrier.dstStageMask = Utility::GetStageFromBarrierStage(barrierInfo.dstStage);
			outBarrier.newLayout = Utility::GetVkImageLayoutFromImageLayout(barrierInfo.dstLayout);
		}

		outBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		outBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		outBarrier.subresourceRange.aspectMask = aspectFlags;
		outBarrier.subresourceRange.baseArrayLayer = barrierInfo.subResource.baseArrayLayer;
		outBarrier.subresourceRange.baseMipLevel = barrierInfo.subResource.baseMipLevel;
		outBarrier.subresourceRange.layerCount = barrierInfo.subResource.layerCount == ALL_LAYERS ? VK_REMAINING_ARRAY_LAYERS : barrierInfo.subResource.layerCount;
		outBarrier.subresourceRange.levelCount = barrierInfo.subResource.levelCount == ALL_MIPS ? VK_REMAINING_MIP_LEVELS : barrierInfo.subResource.levelCount;
		outBarrier.image = barrierInfo.resource->GetHandle<VkImage>();

		GraphicsContext::GetResourceStateTracker()->TransitionResource(barrierInfo.resource, barrierInfo.dstStage, barrierInfo.dstAccess, barrierInfo.dstLayout);
	}

	void VulkanCommandBuffer::ResourceBarrier(const BarrierVector& resourceBarriers)
	{
		using ImageBarrierVector = Vector<VkImageMemoryBarrier2, InlineAllocator<16>>;
		using BufferBarrierVector = Vector<VkBufferMemoryBarrier2, InlineAllocator<16>>;
		using GlobalBarrierVector = Vector<VkMemoryBarrier2, InlineAllocator<16>>;

		ImageBarrierVector imageBarriers{};
		BufferBarrierVector bufferBarriers{};
		GlobalBarrierVector memoryBarriers{};

		for (const auto& resourceBarrier : resourceBarriers)
		{
			VT_ENSURE(resourceBarrier.type != BarrierType::None);

			switch (resourceBarrier.type)
			{
				case BarrierType::Global:
					AddGlobalBarrier(resourceBarrier.globalBarrier(), memoryBarriers.emplace_back());
					break;

				case BarrierType::Buffer:
					AddBufferBarrier(resourceBarrier.bufferBarrier(), bufferBarriers.emplace_back());
					break;

				case BarrierType::Image:
					AddImageBarrier(resourceBarrier.imageBarrier(), imageBarriers.emplace_back());
					break;
			}
		}

		VkDependencyInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		info.pNext = nullptr;
		info.dependencyFlags = 0;
		info.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
		info.pMemoryBarriers = memoryBarriers.data();
		info.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
		info.pBufferMemoryBarriers = bufferBarriers.data();
		info.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
		info.pImageMemoryBarriers = imageBarriers.data();

		vkCmdPipelineBarrier2(m_commandBufferData.commandBuffer, &info);
	}

	void VulkanCommandBuffer::BuildAccelerationStructures(const Vector<AccelerationStructureBuildGeometryInfo>& buildInfos, const Vector<AccelerationStructureBuildRanges>& buildRanges)
	{
		Vector<VkAccelerationStructureGeometryKHR> geometries;
		Vector<VkAccelerationStructureBuildGeometryInfoKHR> buildGeometries;

		Vector<RefPtr<StorageBuffer>> scratchBuffers;

		auto device = GraphicsContext::GetDevice();

		for (const auto& buildInfo : buildInfos)
		{
			size_t offset = geometries.size();
			Vector<uint32_t> primitiveCounts;

			for (const auto& geometryInfo : buildInfo.geometries)
			{
				auto& vulkanGeometry = geometries.emplace_back();
				vulkanGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
				vulkanGeometry.pNext = nullptr;
				vulkanGeometry.geometryType = Utility::GetGeometryType(geometryInfo.geometryType);
				vulkanGeometry.flags = Utility::GetGeometryFlags(geometryInfo.flags);
			
				if (geometryInfo.geometryType == AccelerationStructureGeometryType::Triangles)
				{
					VT_ENSURE(geometryInfo.indexBuffer && geometryInfo.vertexPositionsBuffer);

					auto& triangles = vulkanGeometry.geometry.triangles;
					triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					triangles.pNext = nullptr;
					triangles.vertexFormat = Utility::VoltToVulkanFormat(geometryInfo.vertexFormat);
					triangles.vertexData.deviceAddress = geometryInfo.vertexPositionsBuffer->GetDeviceAddress();
					triangles.vertexStride = geometryInfo.vertexStride;
					triangles.maxVertex = geometryInfo.vertexCount;
					triangles.indexType = Utility::VoltToVulkanIndexType(geometryInfo.indexType);
					triangles.indexData.deviceAddress = geometryInfo.indexBuffer->GetDeviceAddress();
					triangles.transformData.deviceAddress = 0;

					primitiveCounts.emplace_back(geometryInfo.indexBuffer->GetCount() / 3u);
				}
				else if (geometryInfo.geometryType == AccelerationStructureGeometryType::Instances)
				{
					VT_ENSURE(geometryInfo.instancesBuffer);

					auto& instances = vulkanGeometry.geometry.instances;
					instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
					instances.pNext = nullptr;
					instances.arrayOfPointers = VK_FALSE;
					instances.data.deviceAddress = geometryInfo.instancesBuffer->GetDeviceAddress();

					primitiveCounts.emplace_back(geometryInfo.instancesBuffer->GetCount());
				}
			}

			auto& vulkanBuildInfo = buildGeometries.emplace_back();
			vulkanBuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			vulkanBuildInfo.pNext = nullptr;
			vulkanBuildInfo.type = Utility::GetAccelerationStructureType(buildInfo.type);
			vulkanBuildInfo.flags = Utility::GetAccelerationStructureBuildFlags(buildInfo.flags);
			vulkanBuildInfo.mode = Utility::GetAccelerationStructureBuildMode(buildInfo.mode);
			vulkanBuildInfo.geometryCount = static_cast<uint32_t>(geometries.size() - offset);
			vulkanBuildInfo.pGeometries = &geometries[offset];
			vulkanBuildInfo.scratchData.deviceAddress = 0;

			if (buildInfo.mode == AccelerationStructureBuildMode::Update)
			{
				VT_ENSURE(buildInfo.srcAccelerationStructure && buildInfo.dstAccelerationStructure);
			}
			else
			{
				VT_ENSURE(buildInfo.dstAccelerationStructure);
			}
			
			if (buildInfo.srcAccelerationStructure)
			{
				vulkanBuildInfo.srcAccelerationStructure = buildInfo.srcAccelerationStructure->GetHandle<VkAccelerationStructureKHR>();
			}

			if (buildInfo.dstAccelerationStructure)
			{
				vulkanBuildInfo.dstAccelerationStructure = buildInfo.dstAccelerationStructure->GetHandle<VkAccelerationStructureKHR>();
			}

			VkAccelerationStructureBuildSizesInfoKHR buildSizes;
			buildSizes.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
			buildSizes.pNext = nullptr;
			buildSizes.accelerationStructureSize = 0;
			buildSizes.updateScratchSize = 0;
			buildSizes.buildScratchSize = 0;
		
			vkGetAccelerationStructureBuildSizesKHR(device->GetHandle<VkDevice>(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &vulkanBuildInfo, primitiveCounts.data(), &buildSizes);
		
			// #TODO_Ivar: Probably should not get this every time. Maybe these properties should be moved to a private global variable?
			const auto& accelerationStructureProperties = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>()->GetDeviceProperties().accelerationStructureProperties;

			const VkDeviceSize scratchBufferSize = (buildInfo.mode == AccelerationStructureBuildMode::Build ? buildSizes.buildScratchSize : buildSizes.updateScratchSize) + accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;

			BufferDesc scratchBufferDesc{};
			scratchBufferDesc.count = 1;
			scratchBufferDesc.elementSize = scratchBufferSize;
			scratchBufferDesc.usage = BufferUsage::StorageBuffer | BufferUsage::DeviceAddress;
			scratchBufferDesc.debugName = "AS Scratch Buffer";

			RefPtr<StorageBuffer> scratchBuffer = StorageBuffer::Create(scratchBufferDesc);
			scratchBuffers.push_back(scratchBuffer);

			vulkanBuildInfo.scratchData.deviceAddress = ::Utility::Align(scratchBuffer->GetDeviceAddress(), accelerationStructureProperties.minAccelerationStructureScratchOffsetAlignment);
		}

		Vector<VkAccelerationStructureBuildRangeInfoKHR> vulkanBuildRanges;
		Vector<VkAccelerationStructureBuildRangeInfoKHR*> vulkanBuildRangesPtrs;

		for (const auto& buildRange : buildRanges)
		{
			for (const auto& range : buildRange.GetRanges())
			{
				auto& vulkanRange = vulkanBuildRanges.emplace_back();
				vulkanRange.firstVertex = range.firstVertex;
				vulkanRange.primitiveCount = range.primitiveCount;
				vulkanRange.primitiveOffset = range.primitiveOffset;
				vulkanRange.transformOffset = range.transformOffset;
			}
		}

		size_t offset = 0;

		for (const auto& buildRange : buildRanges)
		{
			auto*& rangePtr = vulkanBuildRangesPtrs.emplace_back();
			rangePtr = &vulkanBuildRanges[offset];

			offset += buildRange.GetRanges().size();
		}

		vkCmdBuildAccelerationStructuresKHR(m_commandBufferData.commandBuffer, static_cast<uint32_t>(buildGeometries.size()), buildGeometries.data(), vulkanBuildRangesPtrs.data());
	}

	void VulkanCommandBuffer::BeginMarker(std::string_view markerLabel, const std::array<float, 4>& markerColor)
	{
		if (Volt::RHI::vkCmdBeginDebugUtilsLabelEXT)
		{
			VkDebugUtilsLabelEXT markerInfo{};
			markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
			markerInfo.pLabelName = markerLabel.data();
			markerInfo.color[0] = markerColor[0];
			markerInfo.color[1] = markerColor[1];
			markerInfo.color[2] = markerColor[2];
			markerInfo.color[3] = markerColor[3];

			Volt::RHI::vkCmdBeginDebugUtilsLabelEXT(m_commandBufferData.commandBuffer, &markerInfo);
		}
	}

	void VulkanCommandBuffer::EndMarker()
	{
		if (Volt::RHI::vkCmdEndDebugUtilsLabelEXT)
		{
			Volt::RHI::vkCmdEndDebugUtilsLabelEXT(m_commandBufferData.commandBuffer);
		}
	}

	const uint32_t VulkanCommandBuffer::BeginTimestamp()
	{
		VT_PROFILE_FUNCTION();

		if (!m_hasTimestampSupport)
		{
			return 0;
		}

		const uint32_t queryId = m_nextAvailableTimestampQuery;
		m_nextAvailableTimestampQuery += 2;

		vkCmdWriteTimestamp2(m_commandBufferData.commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_timestampQueryPool, queryId);
		return queryId;
	}

	void VulkanCommandBuffer::EndTimestamp(uint32_t timestampIndex)
	{
		VT_PROFILE_FUNCTION();

		if (!m_hasTimestampSupport)
		{
			return;
		}

		vkCmdWriteTimestamp2(m_commandBufferData.commandBuffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, m_timestampQueryPool, timestampIndex + 1);
	}

	const float VulkanCommandBuffer::GetExecutionTime(uint32_t timestampIndex) const
	{
		VT_PROFILE_FUNCTION();

		if (!m_hasTimestampSupport)
		{
			return 0.f;
		}

		if (timestampIndex == UINT32_MAX || timestampIndex / 2 >= m_timestampCount / 2)
		{
			return 0.f;
		}

		return m_executionTimes.at(timestampIndex / 2);
	}

	void VulkanCommandBuffer::ClearBufferView(RawPtr<BufferView> bufferView, const uint32_t clearValue)
	{
		VT_PROFILE_FUNCTION();

		VulkanBufferView& vkBufferView = bufferView->AsRef<VulkanBufferView>();
		const BufferViewDesc& viewDesc = vkBufferView.GetDesc();

		vkCmdFillBuffer(m_commandBufferData.commandBuffer, bufferView->GetHandle<VkBuffer>(), viewDesc.offset, viewDesc.size, clearValue);
	}

	void VulkanCommandBuffer::ClearBufferView(RawPtr<BufferView> bufferView, const float clearValue)
	{
		VT_PROFILE_FUNCTION();

		VulkanBufferView& vkBufferView = bufferView->AsRef<VulkanBufferView>();
		const BufferViewDesc& viewDesc = vkBufferView.GetDesc();

		const uint32_t uintClearValue = std::bit_cast<uint32_t>(clearValue);
		vkCmdFillBuffer(m_commandBufferData.commandBuffer, bufferView->GetHandle<VkBuffer>(), viewDesc.offset, viewDesc.size, uintClearValue);
	}

	void VulkanCommandBuffer::ClearImageView(RawPtr<ImageView> imageView, std::array<uint32_t, 4> clearValue)
	{
		VT_PROFILE_FUNCTION();

		const ImageViewDesc& desc = imageView->GetDesc();
		RawPtr<Image> image = desc.image->As<Image>();

		VkImageSubresourceRange subResourceRange{};
		subResourceRange.aspectMask = Utility::GetVkImageAspect(imageView->GetImageAspect());
		subResourceRange.baseArrayLayer = desc.baseArrayLayer;
		subResourceRange.baseMipLevel = desc.baseMipLevel;
		subResourceRange.layerCount = desc.layerCount;
		subResourceRange.levelCount = desc.mipCount;

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image);

		const VkImageLayout layout = EnumValueContainsFlag(currentState.stage, BarrierStage::Clear) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Utility::GetVkImageLayoutFromImageLayout(currentState.layout);

		if ((subResourceRange.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0)
		{
			VkClearColorValue vkClearColor{};
			vkClearColor.uint32[0] = clearValue[0];
			vkClearColor.uint32[1] = clearValue[1];
			vkClearColor.uint32[2] = clearValue[2];
			vkClearColor.uint32[3] = clearValue[3];

			vkCmdClearColorImage(m_commandBufferData.commandBuffer, image->GetHandle<VkImage>(), layout, &vkClearColor, 1, &subResourceRange);
		}
		else
		{
			VkClearDepthStencilValue vkClearColor{};
			vkClearColor.depth = static_cast<float>(clearValue[0]);
			vkClearColor.stencil = clearValue[1];

			vkCmdClearDepthStencilImage(m_commandBufferData.commandBuffer, image->GetHandle<VkImage>(), layout, &vkClearColor, 1, &subResourceRange);
		}
	}

	void VulkanCommandBuffer::ClearImageView(RawPtr<ImageView> imageView, std::array<float, 4> clearValue)
	{
		VT_PROFILE_FUNCTION();

		const ImageViewDesc& desc = imageView->GetDesc();
		RawPtr<Image> image = desc.image->As<Image>();

		VkImageSubresourceRange subResourceRange{};
		subResourceRange.aspectMask = Utility::GetVkImageAspect(imageView->GetImageAspect());
		subResourceRange.baseArrayLayer = desc.baseArrayLayer;
		subResourceRange.baseMipLevel = desc.baseMipLevel;
		subResourceRange.layerCount = desc.layerCount;
		subResourceRange.levelCount = desc.mipCount;

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(image);

		const VkImageLayout layout = EnumValueContainsFlag(currentState.stage, BarrierStage::Clear) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Utility::GetVkImageLayoutFromImageLayout(currentState.layout);

		if ((subResourceRange.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0)
		{
			VkClearColorValue vkClearColor{};
			vkClearColor.float32[0] = clearValue[0];
			vkClearColor.float32[1] = clearValue[1];
			vkClearColor.float32[2] = clearValue[2];
			vkClearColor.float32[3] = clearValue[3];


			vkCmdClearColorImage(m_commandBufferData.commandBuffer, image->GetHandle<VkImage>(), layout, &vkClearColor, 1, &subResourceRange);
		}
		else
		{
			VkClearDepthStencilValue vkClearColor{};
			vkClearColor.depth = clearValue[0];
			vkClearColor.stencil = static_cast<uint32_t>(clearValue[1]);

			vkCmdClearDepthStencilImage(m_commandBufferData.commandBuffer, image->GetHandle<VkImage>(), layout, &vkClearColor, 1, &subResourceRange);
		}
	}

	void VulkanCommandBuffer::CopyBufferRegion(Handle<Allocation> srcResource, const size_t srcOffset, Handle<Allocation> dstResource, const size_t dstOffset, const size_t size)
	{
		VT_PROFILE_FUNCTION();

		VkBufferCopy copy{};
		copy.srcOffset = srcOffset;
		copy.dstOffset = dstOffset;
		copy.size = size;

		vkCmdCopyBuffer(m_commandBufferData.commandBuffer, srcResource->GetResourceHandle<VkBuffer>(), dstResource->GetResourceHandle<VkBuffer>(), 1, &copy);
	}

	void VulkanCommandBuffer::CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip)
	{
		CopyBufferToImage(srcBuffer, dstImage, width, height, depth, 0, 0, 0, mip);
	}

	void VulkanCommandBuffer::CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");

		auto& vkImage = dstImage->AsRef<VulkanImage>();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(vkImage.GetImageAspect());
		region.imageSubresource.mipLevel = mip;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { offsetX, offsetY, offsetZ };
		region.imageExtent = { width, height, depth };

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(dstImage);
		vkCmdCopyBufferToImage(m_commandBufferData.commandBuffer, srcBuffer->GetResourceHandle<VkBuffer>(), dstImage->GetHandle<VkImage>(), Utility::GetVkImageLayoutFromImageLayout(currentState.layout), 1, &region);
	}

	void VulkanCommandBuffer::CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");
		VT_ENSURE_MSG(mip < srcImage->CalculateMipCount(), "Mip level is not valid!");

		auto& vkImage = srcImage->AsRef<VulkanImage>();

		VkBufferImageCopy region{};
		region.bufferOffset = dstOffset;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(vkImage.GetImageAspect());
		region.imageSubresource.mipLevel = mip;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = srcImage->GetLayerCount();

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(srcImage);
		vkCmdCopyImageToBuffer(m_commandBufferData.commandBuffer, srcImage->GetHandle<VkImage>(), Utility::GetVkImageLayoutFromImageLayout(currentState.layout), dstBuffer->GetResourceHandle<VkBuffer>(), 1, &region);
	}

	void VulkanCommandBuffer::CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");

		VulkanImage& srcVkImage = srcImage->AsRef<VulkanImage>();
		VulkanImage& dstVkImage = dstImage->AsRef<VulkanImage>();

		const VkImageAspectFlags srcImageAspect = static_cast<VkImageAspectFlags>(srcVkImage.GetImageAspect());
		const VkImageAspectFlags dstImageAspect = static_cast<VkImageAspectFlags>(dstVkImage.GetImageAspect());

		VkImageCopy2 info{};
		info.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2;
		info.pNext = nullptr;
		info.srcSubresource.aspectMask = srcImageAspect;
		info.srcSubresource.baseArrayLayer = 0;
		info.srcSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
		info.srcSubresource.mipLevel = 0;
		info.dstSubresource.aspectMask = dstImageAspect;
		info.dstSubresource.baseArrayLayer = 0;
		info.dstSubresource.layerCount = VK_REMAINING_ARRAY_LAYERS;
		info.dstSubresource.mipLevel = 0;
		info.srcOffset.x = 0;
		info.srcOffset.y = 0;
		info.srcOffset.z = 0;
		info.dstOffset.x = 0;
		info.dstOffset.y = 0;
		info.dstOffset.z = 0;
		info.extent.width = width;
		info.extent.height = height;
		info.extent.depth = 1;

		const auto& currentSrcState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(srcImage);
		const auto& currentDstState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(dstImage);

		VkCopyImageInfo2 cpyInfo{};
		cpyInfo.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2;
		cpyInfo.pNext = nullptr;
		cpyInfo.srcImage = srcVkImage.GetHandle<VkImage>();
		cpyInfo.srcImageLayout = Utility::GetVkImageLayoutFromImageLayout(currentSrcState.layout);
		cpyInfo.dstImage = dstVkImage.GetHandle<VkImage>();
		cpyInfo.dstImageLayout = Utility::GetVkImageLayoutFromImageLayout(currentDstState.layout);
		cpyInfo.pRegions = &info;
		cpyInfo.regionCount = 1;

		vkCmdCopyImage2(m_commandBufferData.commandBuffer, &cpyInfo);
	}

	void VulkanCommandBuffer::UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData)
	{
		VT_PROFILE_FUNCTION();

		auto& vkImage = dstImage->AsRef<VulkanImage>();

		Vector<VkBufferImageCopy> copyRegions;
		copyRegions.reserve(copyData.copySubData.size());

		uint8_t* stagingPtr = stagingAllocation->Map<uint8_t>();

		uint64_t offset = 0;
		for (const auto& subData : copyData.copySubData)
		{
			auto& newRegion = copyRegions.emplace_back();
			newRegion.bufferOffset = offset;
			newRegion.bufferRowLength = 0;
			newRegion.bufferImageHeight = 0;

			newRegion.imageSubresource.aspectMask = static_cast<VkImageAspectFlags>(vkImage.GetImageAspect());
			newRegion.imageSubresource.mipLevel = subData.subResource.baseMipLevel;
			newRegion.imageSubresource.baseArrayLayer = subData.subResource.baseArrayLayer;
			newRegion.imageSubresource.layerCount = subData.subResource.layerCount;

			newRegion.imageOffset = { 0, 0, 0 };
			newRegion.imageExtent = { subData.width, subData.height, subData.depth };

			memcpy_s(&stagingPtr[offset], stagingAllocation->GetSize(), subData.data, subData.slicePitch);
			offset += subData.slicePitch;
		}

		stagingAllocation->Unmap();

		const auto& currentState = GraphicsContext::GetResourceStateTracker()->GetCurrentResourceState(dstImage);
		vkCmdCopyBufferToImage(m_commandBufferData.commandBuffer, stagingAllocation->GetResourceHandle<VkBuffer>(), dstImage->GetHandle<VkImage>(), Utility::GetVkImageLayoutFromImageLayout(currentState.layout), static_cast<uint32_t>(copyRegions.size()), copyRegions.data());
	}

	const QueueType VulkanCommandBuffer::GetQueueType() const
	{
		return m_queueType;
	}

	const CommandBufferLevel VulkanCommandBuffer::GetCommandBufferLevel() const
	{
		return m_commandBufferLevel;
	}

	RefPtr<CommandBuffer> VulkanCommandBuffer::CreateSecondaryCommandBuffer() const
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);
		return RefPtr<VulkanCommandBuffer>::Create(this);
	}

	void VulkanCommandBuffer::ExecuteSecondaryCommandBuffer(RefPtr<CommandBuffer> commandBuffer) const
	{
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);
		VT_ENSURE(commandBuffer->GetCommandBufferLevel() == CommandBufferLevel::Secondary);

		VkCommandBuffer cmdBuffer = commandBuffer->GetHandle<VkCommandBuffer>();
		vkCmdExecuteCommands(m_commandBufferData.commandBuffer, 1, &cmdBuffer);
	}

	void VulkanCommandBuffer::ExecuteSecondaryCommandBuffers(Vector<RefPtr<CommandBuffer>> commandBuffers) const
	{
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);

		Vector<VkCommandBuffer> vkCommandBuffers;
		vkCommandBuffers.reserve(commandBuffers.size());

		for (const auto& cmdBuffer : commandBuffers)
		{
			VT_ENSURE(cmdBuffer->GetCommandBufferLevel() == CommandBufferLevel::Secondary);
			vkCommandBuffers.emplace_back(cmdBuffer->GetHandle<VkCommandBuffer>());
		}

		vkCmdExecuteCommands(m_commandBufferData.commandBuffer, static_cast<uint32_t>(vkCommandBuffers.size()), vkCommandBuffers.data());
	}

	void* VulkanCommandBuffer::GetHandleImpl() const
	{
		return m_commandBufferData.commandBuffer;
	}

	void VulkanCommandBuffer::Invalidate()
	{
		VT_PROFILE_FUNCTION();

		auto physicalDevice = GraphicsContext::GetPhysicalDevice()->As<VulkanPhysicalGraphicsDevice>();
		auto device = GraphicsContext::GetDevice();

		const auto& queueFamilies = physicalDevice->GetQueueFamilies();

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;

		uint32_t queueFamilyIndex = 0;

		switch (m_queueType)
		{
			case QueueType::Graphics:
				queueFamilyIndex = queueFamilies.graphicsFamilyQueueIndex;
				break;
			case QueueType::Compute:
				queueFamilyIndex = queueFamilies.computeFamilyQueueIndex;
				break;
			case QueueType::TransferCopy:
				queueFamilyIndex = queueFamilies.transferFamilyQueueIndex;
				break;
			default:
				VT_ASSERT(false);
				break;
		}

		poolInfo.queueFamilyIndex = queueFamilyIndex;
		poolInfo.flags = 0;

		VT_VK_CHECK(vkCreateCommandPool(device->GetHandle<VkDevice>(), &poolInfo, VT_VULKAN_ALLOCATOR, &m_commandBufferData.commandPool));

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_commandBufferData.commandPool;
		allocInfo.level = m_commandBufferLevel == CommandBufferLevel::Primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;

		VT_VK_CHECK(vkAllocateCommandBuffers(device->GetHandle<VkDevice>(), &allocInfo, &m_commandBufferData.commandBuffer));

		FenceCreateInfo fenceInfo{};
		fenceInfo.createSignaled = true;

		m_hasTimestampSupport = false; //GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>().GetProperties().limits.timestampComputeAndGraphics;
		if (m_hasTimestampSupport)
		{
			CreateQueryPools();
		}
	}

	void VulkanCommandBuffer::Release()
	{
		VT_PROFILE_FUNCTION();

		if (!m_commandBufferData.commandBuffer)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([commandPool = m_commandBufferData.commandPool, timestampPool = m_timestampQueryPool, level = m_commandBufferLevel]()
		{
			auto device = GraphicsContext::GetDevice();

			vkDestroyCommandPool(device->GetHandle<VkDevice>(), commandPool, VT_VULKAN_ALLOCATOR);
			
			if (timestampPool)
			{
				vkDestroyQueryPool(device->GetHandle<VkDevice>(), timestampPool, VT_VULKAN_ALLOCATOR);
			}
		});

		m_commandBufferData = {};
	}

	void VulkanCommandBuffer::CreateQueryPools()
	{
		VT_PROFILE_FUNCTION();

		auto device = GraphicsContext::GetDevice();

		m_timestampQueryCount = 2 + 2 * MAX_QUERIES;

		VkQueryPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		info.queryCount = m_timestampQueryCount;

		VT_VK_CHECK(vkCreateQueryPool(device->GetHandle<VkDevice>(), &info, VT_VULKAN_ALLOCATOR, &m_timestampQueryPool));

		m_timestampCount = 0u;

		m_timestampQueryResults.resize_uninitialized(m_timestampQueryCount);
		m_executionTimes.resize_uninitialized(m_timestampQueryCount / 2);
	}

	void VulkanCommandBuffer::FetchTimestampResults()
	{
		VT_PROFILE_FUNCTION();

		if (!m_hasTimestampSupport)
		{
			return;
		}

		if (m_timestampCount == 0)
		{
			return;
		}

		auto device = GraphicsContext::GetDevice();

		vkGetQueryPoolResults(device->GetHandle<VkDevice>(), m_timestampQueryPool, 0, m_timestampCount, m_timestampCount * sizeof(uint64_t), m_timestampQueryResults.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

		for (uint32_t i = 0; i < m_timestampCount; i += 2)
		{
			const uint64_t startTime = m_timestampQueryResults.at(i);
			const uint64_t endTime = m_timestampQueryResults.at(i + 1);

			const float nsTime = endTime > startTime ? (endTime - startTime) * GraphicsContext::GetPhysicalDevice()->AsRef<VulkanPhysicalGraphicsDevice>().GetProperties().limits.timestampPeriod : 0.f;
			m_executionTimes[i / 2] = nsTime * 0.000001f; // Convert to ms
		}
	}

	void VulkanCommandBuffer::BeginPrimaryInternal(bool oneTimeSubmit)
	{
		VT_PROFILE_FUNCTION();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
		beginInfo.pInheritanceInfo = nullptr;

		VT_VK_CHECK(vkBeginCommandBuffer(m_commandBufferData.commandBuffer, &beginInfo));
	}

	void VulkanCommandBuffer::BeginSecondaryInternal(bool oneTimeSubmit)
	{
		VT_PROFILE_FUNCTION();

		VkCommandBufferInheritanceInfo inheritanceInfo{};
		inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
		inheritanceInfo.pNext = nullptr;
		inheritanceInfo.renderPass = nullptr;
		inheritanceInfo.subpass = 0;
		inheritanceInfo.framebuffer = nullptr;
		inheritanceInfo.occlusionQueryEnable = VK_FALSE;
		inheritanceInfo.queryFlags = 0;
		inheritanceInfo.pipelineStatistics = 0;

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = oneTimeSubmit ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
		beginInfo.pInheritanceInfo = &inheritanceInfo;

		VT_VK_CHECK(vkBeginCommandBuffer(m_commandBufferData.commandBuffer, &beginInfo));
	}

	void VulkanCommandBuffer::ClearCurrentPipeline()
	{
		m_currentRayTracingPipeline.Reset();
		m_currentComputePipeline.Reset();
		m_currentRenderPipeline.Reset();
	}
}
