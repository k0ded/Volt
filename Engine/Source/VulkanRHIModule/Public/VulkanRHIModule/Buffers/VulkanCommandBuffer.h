#pragma once

#include "VulkanRHIModule/Core.h"
#include <RHIModule/Buffers/CommandBuffer.h>

struct VkCommandBuffer_T;
struct VkCommandPool_T;
struct VkFence_T;

struct VkQueryPool_T;
struct VkPipelineLayout_T;

namespace Volt::RHI
{
	class Semaphore;

	class VulkanCommandBuffer final : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(QueueType queueType);
		VulkanCommandBuffer(const CommandBuffer* parentCommandBuffer);
		~VulkanCommandBuffer() override;

		void Begin() override;
		void End() override;

		void SetEvent(RawPtr<Event> event) override;

		void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance) override;
		void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance) override;
		void DrawIndexedIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndexedIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;
		void DrawIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset) override;

		void DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchMeshTasksIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DispatchMeshTasksIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void SetViewports(const StackVector<Viewport, MAX_VIEWPORT_COUNT>& viewports) override;
		void SetScissors(const StackVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors) override;

		void BindPipeline(RawPtr<RenderPipeline> pipeline) override;
		void BindPipeline(RawPtr<ComputePipeline> pipeline) override;
		void BindPipeline(RawPtr<RayTracingPipeline> pipeline) override;
		void BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding) override;
		void BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer, const IndexType indexType) override;

		void BindDescriptorTable(RawPtr<DescriptorTable> descriptorTable) override;
		void BindDescriptorTable(RawPtr<BindlessDescriptorTable> descriptorTable, RawPtr<UniformBuffer> constantsBuffer, const uint32_t offsetIndex, const uint32_t stride, RawPtr<AccelerationStructure> accelerationStructure) override;

		void BeginRendering(const RenderingInfo& renderingInfo) override;
		void EndRendering() override;

		void ResourceBarrier(const BarrierVector& resourceBarriers) override;

		void BuildAccelerationStructures(const Vector<AccelerationStructureBuildGeometryInfo>& buildInfos, const Vector<AccelerationStructureBuildRanges>& buildRanges) override;

		void BeginMarker(std::string_view markerLabel, const std::array<float, 4>& markerColor) override;
		void EndMarker() override;

		const uint32_t BeginTimestamp() override;
		void EndTimestamp(uint32_t timestampIndex) override;
		const float GetExecutionTime(uint32_t timestampIndex) const override;

		void ClearBufferView(RawPtr<BufferView> bufferView, const uint32_t clearValue) override;
		void ClearBufferView(RawPtr<BufferView> bufferView, const float clearValue) override;

		void ClearImageView(RawPtr<ImageView> imageView, std::array<uint32_t, 4> clearValue) override;
		void ClearImageView(RawPtr<ImageView> imageView, std::array<float, 4> clearValue) override;

		void CopyBufferRegion(Handle<Allocation> srcAllocation, const size_t srcOffset, Handle<Allocation> dstAllocation, const size_t dstOffset, const size_t size) override;
		void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip /* = 0 */) override;
		void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip) override;
		void CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip) override;
		void CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData) override;

		const QueueType GetQueueType() const override;
		const CommandBufferLevel GetCommandBufferLevel() const override;

		RefPtr<CommandBuffer> CreateSecondaryCommandBuffer() const override;
		void ExecuteSecondaryCommandBuffer(RefPtr<CommandBuffer> commandBuffer) const override;
		void ExecuteSecondaryCommandBuffers(Vector<RefPtr<CommandBuffer>> commandBuffers) const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class VulkanDescriptorTable;
		friend class VulkanDescriptorBufferTable;
		friend class VulkanBindlessDescriptorTable;

		inline static constexpr uint32_t MAX_QUERIES = 64;

		void Invalidate();
		void Release();

		void CreateQueryPools();
		void FetchTimestampResults();

		void BeginPrimaryInternal();
		void BeginSecondaryInternal();

		void ClearCurrentPipeline();

		struct CommandBufferData
		{
			VkCommandBuffer_T* commandBuffer = nullptr;
			VkCommandPool_T* commandPool = nullptr;
		};

		CommandBufferData m_commandBufferData;

		bool m_hasTimestampSupport = false;

		QueueType m_queueType;

		// Queries
		uint32_t m_timestampQueryCount = 0;
		uint32_t m_nextAvailableTimestampQuery = 0; // The two first are command buffer total
		uint32_t m_lastAvailableTimestampQuery = 0;

		VkQueryPool_T* m_timestampQueryPool = nullptr;
		uint32_t m_timestampCount;
		Vector<uint64_t> m_timestampQueryResults;
		Vector<float> m_executionTimes;

		// Internal state
		RawPtr<RenderPipeline> m_currentRenderPipeline;
		RawPtr<ComputePipeline> m_currentComputePipeline;
		RawPtr<RayTracingPipeline> m_currentRayTracingPipeline;

		// Secondary command buffer
		CommandBufferLevel m_commandBufferLevel = CommandBufferLevel::Primary;
		const CommandBuffer* m_parentCommandBuffer;
	};
}
