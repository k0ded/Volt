#pragma once

#include "VulkanRHIModule/LastSubmissionTrackerManager.h"
#include "VulkanRHIModule/Utility/DescriptorSetLayoutBuilder.h"

#include <RHIModule/Descriptors/ResourceTable.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/Buffers/CommandBuffer.h>
#include <RHIModule/Core/RenderingInfo.h>

struct VkCommandBuffer_T;
struct VkCommandPool_T;
struct VkFence_T;

struct VkQueryPool_T;
struct VkPipelineLayout_T;
struct VkSemaphore_T;

namespace Volt::RHI
{
	class Semaphore;

	class VulkanCommandBuffer final : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(QueueType queueType);
		VulkanCommandBuffer(const CommandBuffer* parentCommandBuffer);

		VulkanCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration);

		~VulkanCommandBuffer() override;

		void Begin(bool oneTimeSubmit) override;
		void End() override;
		                                                  
		void Reset() override;

		void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance) override;
		void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance) override;
		void DrawIndexedIndirect(RawPtr<Buffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndirect(RawPtr<Buffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndexedIndirectCount(RawPtr<Buffer> commandsBuffer, const size_t offset, RawPtr<Buffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;
		void DrawIndirectCount(RawPtr<Buffer> commandsBuffer, const size_t offset, RawPtr<Buffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchIndirect(RawPtr<Buffer> commandsBuffer, const size_t offset) override;

		void DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchMeshTasksIndirect(RawPtr<Buffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DispatchMeshTasksIndirectCount(RawPtr<Buffer> commandsBuffer, const size_t offset, RawPtr<Buffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void SetViewports(const InlineVector<Viewport, MAX_VIEWPORT_COUNT>& viewports) override;
		void SetScissors(const InlineVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors) override;

		void BindPipeline(RawPtr<RenderPipeline> pipeline) override;
		void BindPipeline(RawPtr<ComputePipeline> pipeline) override;
		void BindPipeline(RawPtr<RayTracingPipeline> pipeline) override;
		void BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding) override;
		void BindIndexBuffer(RawPtr<Buffer> indexBuffer, const IndexType indexType) override;

		void BindShaderBindings(const ShaderBindingMap& shaderBindingsMap) override;
		void PushInlineParameters(const void* data, const uint32_t size, const uint32_t offset, ShaderStage shaderStages) override;

		void BeginRendering(const RenderingInfo& renderingInfo) override;
		void EndRendering() override;

		void ResourceBarrier(const BarrierVector& resourceBarriers) override;

		void BuildAccelerationStructures(const Vector<AccelerationStructureBuildGeometryInfo>& buildInfos, const Vector<AccelerationStructureBuildRanges>& buildRanges) override;

		void BeginMarker(StringView markerLabel, const std::array<float, 4>& markerColor) override;
		void EndMarker() override;

		const uint32_t BeginTimestamp() override;
		void EndTimestamp(uint32_t timestampIndex) override;
		const float GetExecutionTime(uint32_t timestampIndex) const override;

		void ClearBufferView(RawPtr<BufferView> bufferView, const uint32_t clearValue) override;
		void ClearBufferView(RawPtr<BufferView> bufferView, const float clearValue) override;

		void ClearImageView(RawPtr<ImageView> imageView, std::array<uint32_t, 4> clearValue) override;
		void ClearImageView(RawPtr<ImageView> imageView, std::array<float, 4> clearValue) override;

		void CopyBufferRegion(RawPtr<Buffer> srcBuffer, const size_t srcOffset, RawPtr<Buffer> dstBuffer, const size_t dstOffset, const size_t size) override;
		void CopyBufferToImage(RawPtr<Buffer> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip /* = 0 */) override;
		void CopyBufferToImage(RawPtr<Buffer> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip) override;
		void CopyImageToBuffer(RawPtr<Image> srcImage, RawPtr<Buffer> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip) override;
		void CopyImageToBuffer(RawPtr<Image> srcImage, RawPtr<Buffer> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip) override;
		void CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void UploadTextureData(RawPtr<Image> dstImage, RawPtr<Buffer> stagingAllocation, const ImageCopyData& copyData) override;

		bool HasFinishedExecution() const override;

		const QueueType GetQueueType() const override;
		const CommandBufferLevel GetCommandBufferLevel() const override;

		IntRef<CommandBuffer> CreateSecondaryCommandBuffer() const override;
		void ExecuteSecondaryCommandBuffer(IntRef<CommandBuffer> commandBuffer) const override;
		void ExecuteSecondaryCommandBuffers(Vector<IntRef<CommandBuffer>> commandBuffers) const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class VulkanDeviceQueue;

		inline static constexpr uint32_t MAX_QUERIES = 64;

		void Invalidate();
		void Release();

		void CreateQueryPools();
		void FetchTimestampResults();

		void BeginPrimaryInternal(bool oneTimeSubmit);
		void BeginSecondaryInternal(bool oneTimeSubmit);

		void BindDescriptorBuffer(IntRef<ResourceTable> rayTracingResourceTable);

		void ClearActivePipeline();
		void ValidateInlineParameters();

		void AssignSemaphore(VkSemaphore_T* semaphore, uint64_t value);

		template<typename T>
		void RegisterUsage(RawPtr<T> resource);

		template<typename T>
		void RegisterUsage(IntRef<T> resource);

		VkPipelineLayout_T* GetActivePipelineLayout();
		const DescriptorSetLayoutBuilder::DescriptorSets& GetActivePipelineDescriptorSets();

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
		RawPtr<RenderPipeline> m_activeRenderPipeline;
		RawPtr<ComputePipeline> m_activeComputePipeline;
		RawPtr<RayTracingPipeline> m_activeRayTracingPipeline;
		IntRef<Fence> m_submissionFence;

		// Semaphores that should be waited on when this command buffer is executed.
		InlineVector<VkSemaphore_T*, 1> m_waitSemaphores;

		// Secondary command buffer
		CommandBufferLevel m_commandBufferLevel = CommandBufferLevel::Primary;
		RenderingAttachmentDeclaration m_renderingAttachmentDeclaraion;
		bool m_hasRenderingAttachmentDeclaration = false;
		const CommandBuffer* m_parentCommandBuffer;

		// Submission tracking
		LastSubmissionTrackerManager m_lastSubmissionTrackerManager;
	};
}
