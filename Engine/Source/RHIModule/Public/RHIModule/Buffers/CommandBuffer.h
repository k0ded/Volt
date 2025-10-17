#pragma once

#include "RHIModule/Core/RHIInterface.h"
#include "RHIModule/Core/RHICommon.h"

#include "RHIModule/Pipelines/RayTracingPipeline.h"
#include "RHIModule/Pipelines/RenderPipeline.h"
#include "RHIModule/Pipelines/ComputePipeline.h"

#include "RHIModule/RayTracing/RayTracingCommon.h"
#include "RHIModule/RayTracing/ShaderBindingTable.h"

#include <CoreUtilities/Pointers/RawPtr.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Allocators/Handle.h>
#include <CoreUtilities/Containers/VectorVariants.h>

namespace Volt::RHI
{
	class Image;
	class StorageBuffer;
	class Allocation;
	class Swapchain;
	class ShaderBindingMap;
	class BufferView;
	class ImageView;

	class Event;
	class Fence;

	class AccelerationStructure;

	struct RenderingInfo;

	enum class CommandBufferLevel
	{
		Primary,
		Secondary
	};

	struct VertexBufferBinding
	{
		RefPtr<RHI::StorageBuffer> buffer;
		uint64_t offset = 0;
	};

	using BarrierVector = Vector<ResourceBarrierInfo, InlineAllocator<32>>;
	using VertexBufferVector = Vector<VertexBufferBinding, InlineAllocator<MAX_VERTEX_BUFFER_COUNT>>;

	class VTRHI_API CommandBuffer : public RHIInterface
	{
	public:
		VT_DELETE_COPY_MOVE(CommandBuffer);
		~CommandBuffer() override = default;

		virtual void Begin(bool oneTimeSubmit = true) = 0;
		virtual void End() = 0;

		virtual void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance) = 0;
		virtual void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance) = 0;
		virtual void DrawIndexedIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t maxDrawCount, const uint32_t stride) = 0;
		virtual void DrawIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t maxDrawCount, const uint32_t stride) = 0;
		virtual void DrawIndexedIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) = 0;
		virtual void DrawIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) = 0;

		virtual void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) = 0;
		virtual void DispatchIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset) = 0;

		virtual void DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) = 0;
		virtual void DispatchMeshTasksIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride) = 0;
		virtual void DispatchMeshTasksIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride) = 0;

		virtual void TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth) = 0;

		virtual void SetViewports(const InlineVector<Viewport, MAX_VIEWPORT_COUNT>& viewports) = 0;
		virtual void SetScissors(const InlineVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors) = 0;

		virtual void BindPipeline(RawPtr<RenderPipeline> pipeline) = 0;
		virtual void BindPipeline(RawPtr<ComputePipeline> pipeline) = 0;
		virtual void BindPipeline(RawPtr<RayTracingPipeline> pipeline) = 0;
		virtual void BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding) = 0;
		virtual void BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer, const IndexType indexType = IndexType::UInt32) = 0;

		virtual void BindShaderBindings(const ShaderBindingMap& shaderBindings) = 0;

		virtual void BeginRendering(const RenderingInfo& renderingInfo) = 0;
		virtual void EndRendering() = 0;

		virtual void ResourceBarrier(const BarrierVector& resourceBarriers) = 0;

		virtual void BuildAccelerationStructures(const Vector<AccelerationStructureBuildGeometryInfo>& buildInfos, const Vector<AccelerationStructureBuildRanges>& buildRanges) = 0;

		virtual void BeginMarker(std::string_view markerLabel, const std::array<float, 4>& markerColor) = 0;
		virtual void EndMarker() = 0;

		virtual const uint32_t BeginTimestamp() = 0;
		virtual void EndTimestamp(uint32_t timestampIndex) = 0;
		virtual const float GetExecutionTime(uint32_t timestampIndex) const = 0;

		virtual void ClearBufferView(RawPtr<BufferView> bufferView, const float clearValue) = 0;
		virtual void ClearBufferView(RawPtr<BufferView> bufferView, const uint32_t clearValue) = 0;

		virtual void ClearImageView(RawPtr<ImageView> imageView, std::array<float, 4> clearValue) = 0;
		virtual void ClearImageView(RawPtr<ImageView> imageView, std::array<uint32_t, 4> clearValue) = 0;

		virtual void CopyBufferRegion(Handle<Allocation> srcResource, const size_t srcOffset, Handle<Allocation> dstResource, const size_t dstOffset, const size_t size) = 0;
		virtual void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip = 0) = 0;
		virtual void CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip = 0) = 0;
		virtual void CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip) = 0;
		virtual void CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth) = 0;

		virtual void UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData) = 0;

		virtual bool HasFinishedExecution() const = 0;

		virtual const QueueType GetQueueType() const = 0;
		virtual const CommandBufferLevel GetCommandBufferLevel() const = 0;

		virtual RefPtr<CommandBuffer> CreateSecondaryCommandBuffer() const = 0;
		virtual void ExecuteSecondaryCommandBuffer(RefPtr<CommandBuffer> commandBuffer) const = 0;
		virtual void ExecuteSecondaryCommandBuffers(Vector<RefPtr<CommandBuffer>> commandBuffers) const = 0;

		static RefPtr<CommandBuffer> Create(QueueType queueType);
		static RefPtr<CommandBuffer> Create();

	protected:
		CommandBuffer() = default;
	};
}
