#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"

#include <RHIModule/Buffers/CommandBuffer.h>

struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList8;

namespace Volt::RHI
{
	class D3D12CommandBuffer : public CommandBuffer
	{
	public:
		D3D12CommandBuffer(QueueType queueType);
		D3D12CommandBuffer(const CommandBuffer* parentCommandBuffer);
		~D3D12CommandBuffer() override;

		void Begin(bool oneTimeSubmit) override;
		void End() override;

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

		void SetViewports(const InlineVector<Viewport, MAX_VIEWPORT_COUNT>& viewports) override;
		void SetScissors(const InlineVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors) override;

		void BindPipeline(RawPtr<RenderPipeline> pipeline) override;
		void BindPipeline(RawPtr<ComputePipeline> pipeline) override;
		void BindPipeline(RawPtr<RayTracingPipeline> pipeline) override;
		void BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding) override;
		void BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer, const IndexType indexType) override;

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

		void CopyBufferRegion(Handle<Allocation> srcAllocation, const size_t srcOffset, Handle<Allocation> dstAllocation, const size_t dstOffset, const size_t size) override;
		void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip /* = 0 */) override;
		void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip) override;
		void CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip) override;
		void CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData) override;

		bool HasFinishedExecution() const override;

		const QueueType GetQueueType() const override;
		const CommandBufferLevel GetCommandBufferLevel() const override;

		IntRef<CommandBuffer> CreateSecondaryCommandBuffer() const override;
		void ExecuteSecondaryCommandBuffer(IntRef<CommandBuffer> commandBuffer) const override;
		void ExecuteSecondaryCommandBuffers(Vector<IntRef<CommandBuffer>> commandBuffers) const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class D3D12DeviceQueue;

		void Invalidate();
		void Release();

		void BindDescriptorHeaps();

		void ClearActivePipeline();

		struct CommandListData
		{
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			ComPtr<ID3D12GraphicsCommandList7> commandList;
		};

		CommandListData m_commandListData;

		QueueType m_queueType;

		// Internal state
		RawPtr<RenderPipeline> m_activeRenderPipeline;
		RawPtr<ComputePipeline> m_activeComputePipeline;
		RawPtr<RayTracingPipeline> m_activeRayTracingPipeline;
		IntRef<Fence> m_submissionFence;

		// Secondary command buffer
		CommandBufferLevel m_commandBufferLevel = CommandBufferLevel::Primary;
		const CommandBuffer* m_parentCommandBuffer;
	};
}
