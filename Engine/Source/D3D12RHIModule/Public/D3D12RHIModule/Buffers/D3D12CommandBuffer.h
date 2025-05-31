#pragma once

#include "D3D12RHIModule/Common/ComPtr.h"
#include "D3D12RHIModule/Descriptors/DescriptorCommon.h"

#include <RHIModule/Synchronization/Semaphore.h>
#include <RHIModule/Buffers/CommandBuffer.h>

struct ID3D12Fence;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList6;

namespace Volt::RHI
{
	class D3D12DescriptorHeap;
	class D3D12CommandBuffer final : public CommandBuffer
	{
	public:
		D3D12CommandBuffer(QueueType queueType);
		~D3D12CommandBuffer() override;

		void Begin() override;
		void End() override;

		void Flush(RefPtr<Fence> fence) override;
		void Execute() override;
		void ExecuteAndWait() override;
		void ExecuteWithFence(RefPtr<Fence> fence) override;
		void WaitForFence() override;
		void SetEvent(RawPtr<Event> event) override;

		void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance) override;
		void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance) override;
		void DrawIndexedIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DrawIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;
		void DrawIndexedIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchMeshTasksIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) override;
		void DispatchMeshTasksIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) override;

		void TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) override;
		void DispatchIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset) override;

		void SetViewports(const StackVector<Viewport, MAX_VIEWPORT_COUNT>& viewports) override;
		void SetScissors(const StackVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors) override;

		void BindPipeline(RawPtr<RenderPipeline> pipeline) override;
		void BindPipeline(RawPtr<ComputePipeline> pipeline) override;
		void BindPipeline(RawPtr<RayTracingPipeline> pipeline) override;

		void BindVertexBuffers(const StackVector<RawPtr<VertexBuffer>, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding) override;
		void BindVertexBuffers(const StackVector<RawPtr<StorageBuffer>, RHI::MAX_VERTEX_BUFFER_COUNT>& vertexBuffers, const uint32_t firstBinding) override;
		void BindIndexBuffer(RawPtr<IndexBuffer> indexBuffer) override;
		void BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer) override;

		void BindDescriptorTable(RawPtr<DescriptorTable> descriptorTable) override;
		void BindDescriptorTable2(RawPtr<DescriptorTable> descriptorTable) override;
		void BindDescriptorTable(RawPtr<BindlessDescriptorTable> descriptorTable, RawPtr<UniformBuffer> constantsBuffer, const uint32_t offsetIndex, const uint32_t stride, RawPtr<AccelerationStructure> accelerationStructure) override;

		void BeginRendering(const RenderingInfo& renderingInfo) override;
		void EndRendering() override;

		void PushConstants(const void* data, const uint32_t size, const uint32_t offset) override;

		void ResourceBarrier(const Vector<ResourceBarrierInfo>& resourceBarriers) override;

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

		void CopyBufferRegion(Handle<Allocation> srcResource, const size_t srcOffset, Handle<Allocation> dstResource, const size_t dstOffset, const size_t size) override;
		void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip /* = 0 */) override;
		void CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip) override;
		void CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth) override;

		void UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData) override;

		RefPtr<Semaphore> GetSemaphore() const { return m_commandListData.fence; }

		const QueueType GetQueueType() const override;
		const CommandBufferLevel GetCommandBufferLevel() const override;
		const RawPtr<Fence> GetFence() const override;

		RefPtr<CommandBuffer> CreateSecondaryCommandBuffer() const override;
		void ExecuteSecondaryCommandBuffer(RefPtr<CommandBuffer> commandBuffer) const override;
		void ExecuteSecondaryCommandBuffers(Vector<RefPtr<CommandBuffer>> commandBuffers) const override;

	protected:
		void* GetHandleImpl() const override;

	private:
		friend class D3D12DescriptorTable;
		friend class D3D12BindlessDescriptorTable;

		void Invalidate();
		void Release();

		void BindPipelineInternal();

		D3D12DescriptorPointer CreateTempDescriptorPointer();

		struct CommandListData
		{
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			ComPtr<ID3D12GraphicsCommandList7> commandList;
			RefPtr<Semaphore> fence;
		};

		CommandListData m_commandListData;
		QueueType m_queueType;

		// Internal state
		RawPtr<RenderPipeline> m_currentRenderPipeline;
		RawPtr<ComputePipeline> m_currentComputePipeline;

		bool m_pipelineNeedsToBeBound = false;

		Vector<D3D12DescriptorPointer> m_allocatedDescriptors;
		Scope<D3D12DescriptorHeap> m_descriptorHeap;
	};
}
