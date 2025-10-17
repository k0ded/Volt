#include "dxpch.h"

#include "D3D12RHIModule/Buffers/D3D12CommandBuffer.h"
#include "D3D12RHIModule/Buffers/CommandSignatureCache.h"
#include "D3D12RHIModule/Pipelines/D3D12RenderPipeline.h"
#include "D3D12RHIModule/Pipelines/D3D12ComputePipeline.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Common/D3D12Helpers.h"

#include "D3D12RHIModule/Images/D3D12ImageView.h"
#include "D3D12RHIModule/Images/D3D12SamplerState.h"

#include "D3D12RHIModule/Buffers/D3D12BufferView.h"

#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"

#include <RHIModule/Images/Image.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHIModule.h>
#include <RHIModule/RHIFeatures.h>
#include <RHIModule/Buffers/StorageBuffer.h>
#include <RHIModule/Memory/Allocation.h>
#include <RHIModule/Core/RenderingInfo.h>
#include <RHIModule/Descriptors/ShaderBindingMap.h>

#include <RHIModule/RHIFeatures.h>

#include <CoreUtilities/Profiling/Profiling.h>
#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/EnumUtils.h>

#include <pix.h>

namespace Volt::RHI
{
	namespace Utility
	{
		inline D3D12_BARRIER_SYNC GetD3D12BarrierSyncFromBarrierStage(const BarrierStage barrierStage)
		{
			D3D12_BARRIER_SYNC result = D3D12_BARRIER_SYNC_NONE;

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
				result |= D3D12_BARRIER_SYNC_ALL;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::IndexInput))
			{
				result |= D3D12_BARRIER_SYNC_INDEX_INPUT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VertexInput))
			{
				result |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VertexShader))
			{
				result |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::PixelShader))
			{
				result |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::DepthStencil))
			{
				result |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::RenderTarget))
			{
				result |= D3D12_BARRIER_SYNC_RENDER_TARGET;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::ComputeShader))
			{
				result |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::RayTracingShader))
			{
				result |= D3D12_BARRIER_SYNC_RAYTRACING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::Copy))
			{
				result |= D3D12_BARRIER_SYNC_COPY;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::Resolve))
			{
				result |= D3D12_BARRIER_SYNC_RESOLVE;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::DrawIndirect))
			{
				result |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::AllGraphics))
			{
				result |= D3D12_BARRIER_SYNC_ALL_SHADING;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VideoDecode))
			{
				result |= D3D12_BARRIER_SYNC_VIDEO_DECODE;
			}

			if (EnumValueContainsFlag(barrierStage, BarrierStage::VideoEncode))
			{
				result |= D3D12_BARRIER_SYNC_VIDEO_ENCODE;
			}

			return result;
		}

		inline D3D12_BARRIER_ACCESS GetD3D12BarrierAccessFromBarrierAccess(const BarrierAccess barrierAccess)
		{
			D3D12_BARRIER_ACCESS result = D3D12_BARRIER_ACCESS_COMMON;

			if (barrierAccess == BarrierAccess::None)
			{
				result |= D3D12_BARRIER_ACCESS_NO_ACCESS;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VertexBuffer))
			{
				result |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::UniformBuffer))
			{
				result |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::IndexBuffer))
			{
				result |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::RenderTarget))
			{
				result |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ShaderWrite))
			{
				result |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::DepthStencilWrite))
			{
				result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::DepthStencilRead) && !EnumValueContainsFlag(barrierAccess, BarrierAccess::DepthStencilWrite))
			{
				result |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ShaderRead))
			{
				result |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::IndirectArgument))
			{
				result |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::CopyDest))
			{
				result |= D3D12_BARRIER_ACCESS_COPY_DEST;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::CopySource))
			{
				result |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ResolveDest))
			{
				result |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::ResolveSource))
			{
				result |= D3D12_BARRIER_ACCESS_RESOLVE_SOURCE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoEncodeRead))
			{
				result |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_READ;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoEncodeWrite))
			{
				result |= D3D12_BARRIER_ACCESS_VIDEO_ENCODE_WRITE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoDecodeRead))
			{
				result |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::VideoDecodeWrite))
			{
				result |= D3D12_BARRIER_ACCESS_VIDEO_DECODE_WRITE;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AllRead))
			{
				result |= D3D12_BARRIER_ACCESS_COMMON;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AllWrite))
			{
				result |= D3D12_BARRIER_ACCESS_COMMON;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AccelerationStructureRead))
			{
				result |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
			}

			if (EnumValueContainsFlag(barrierAccess, BarrierAccess::AccelerationStructureWrite))
			{
				result |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
			}

			return result;
		}

		inline D3D12_BARRIER_LAYOUT GetD3D12BarrierLayoutFromImageLayout(const ImageLayout layout)
		{
			switch (layout)
			{
				case ImageLayout::Undefined: return D3D12_BARRIER_LAYOUT_UNDEFINED;
				case ImageLayout::Present: return D3D12_BARRIER_LAYOUT_PRESENT;
				case ImageLayout::RenderTarget: return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
				case ImageLayout::ShaderWrite: return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
				case ImageLayout::DepthStencilWrite: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
				case ImageLayout::DepthStencilRead: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
				case ImageLayout::ShaderRead: return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
				case ImageLayout::CopySource: return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
				case ImageLayout::CopyDest: return D3D12_BARRIER_LAYOUT_COPY_DEST;
				case ImageLayout::ResolveSource: return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
				case ImageLayout::ResolveDest: return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
				case ImageLayout::VideoDecodeRead: return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_READ;
				case ImageLayout::VideoDecodeWrite: return D3D12_BARRIER_LAYOUT_VIDEO_ENCODE_WRITE;
				case ImageLayout::VideoEncodeRead: return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ;
				case ImageLayout::VideoEncodeWrite: return D3D12_BARRIER_LAYOUT_VIDEO_DECODE_READ;
			}

			VT_ENSURE(false);
			return D3D12_BARRIER_LAYOUT_UNDEFINED;
		}
	}

	D3D12CommandBuffer::D3D12CommandBuffer(QueueType queueType)
		: m_queueType(queueType)
	{
		Invalidate();
	}

	D3D12CommandBuffer::D3D12CommandBuffer(const CommandBuffer* parentCommandBuffer)
		: m_queueType(parentCommandBuffer->GetQueueType()), m_commandBufferLevel(CommandBufferLevel::Secondary), m_parentCommandBuffer(parentCommandBuffer)
	{
		Invalidate();
	}

	D3D12CommandBuffer::~D3D12CommandBuffer()
	{
		Release();
	}

	void D3D12CommandBuffer::Begin(bool oneTimeSubmit)
	{
		VT_PROFILE_FUNCTION();

		VT_D3D12_CHECK(m_commandListData.commandAllocator->Reset());
		VT_D3D12_CHECK(m_commandListData.commandList->Reset(m_commandListData.commandAllocator.Get(), nullptr));
	
		BindDescriptorHeaps();
	}

	void D3D12CommandBuffer::End()
	{
		VT_PROFILE_FUNCTION();
		m_commandListData.commandList->Close();
	}

	void D3D12CommandBuffer::Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		m_commandListData.commandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void D3D12CommandBuffer::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex, const uint32_t vertexOffset, const uint32_t firstInstance)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		m_commandListData.commandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void D3D12CommandBuffer::DrawIndexedIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::DrawIndexed, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), drawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, nullptr, 0);
	}

	void D3D12CommandBuffer::DrawIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::Draw, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), drawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, nullptr, 0);
	}

	void D3D12CommandBuffer::DrawIndexedIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::DrawIndexed, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), maxDrawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, countBuffer->GetHandle<ID3D12Resource*>(), countBufferOffset);
	}

	void D3D12CommandBuffer::DrawIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::Draw, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), maxDrawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, countBuffer->GetHandle<ID3D12Resource*>(), countBufferOffset);
	}

	void D3D12CommandBuffer::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeComputePipeline != nullptr);
#endif

		m_commandListData.commandList->Dispatch(groupCountX, groupCountY, groupCountZ);
	}

	void D3D12CommandBuffer::DispatchIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeComputePipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::Dispatch, sizeof(DispatchIndirectCommand));
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), 1, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, nullptr, 0);
	}

	void D3D12CommandBuffer::DispatchMeshTasks(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		m_commandListData.commandList->DispatchMesh(groupCountX, groupCountY, groupCountZ);
	}

	void D3D12CommandBuffer::DispatchMeshTasksIndirect(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, const uint32_t drawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::DispatchMesh, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), drawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, nullptr, 0);
	}

	void D3D12CommandBuffer::DispatchMeshTasksIndirectCount(RawPtr<StorageBuffer> commandsBuffer, const size_t offset, RawPtr<StorageBuffer> countBuffer, const size_t countBufferOffset, const uint32_t maxDrawCount, const uint32_t stride)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRenderPipeline != nullptr);
#endif

		ComPtr<ID3D12CommandSignature> signature = g_commandSignatureCache.GetCommandSignature(CommandSignatureType::DispatchMesh, stride);
		m_commandListData.commandList->ExecuteIndirect(signature.Get(), maxDrawCount, commandsBuffer->GetHandle<ID3D12Resource*>(), offset, countBuffer->GetHandle<ID3D12Resource*>(), countBufferOffset);
	}

	void D3D12CommandBuffer::TraceRays(RawPtr<ShaderBindingTable> shaderBindingTable, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
#ifdef VT_ENABLE_COMMAND_BUFFER_VALIDATION
		VT_ENSURE(m_activeRayTracingPipeline != nullptr);
#endif
	}

	void D3D12CommandBuffer::SetViewports(const InlineVector<Viewport, MAX_VIEWPORT_COUNT>& viewports)
	{
		// The Volt Viewport structure has the same layout as D3D12_VIEWPORT.
		m_commandListData.commandList->RSSetViewports(static_cast<uint32_t>(viewports.size()), reinterpret_cast<const D3D12_VIEWPORT*>(viewports.data()));
	}

	void D3D12CommandBuffer::SetScissors(const InlineVector<Rect2D, MAX_VIEWPORT_COUNT>& scissors)
	{
		InlineVector<D3D12_RECT, MAX_VIEWPORT_COUNT> d3d12Rects;

		for (const Rect2D& rect : scissors)
		{
			D3D12_RECT& d3d12Rect = d3d12Rects.emplace_back();
			d3d12Rect.left = rect.offset.x;
			d3d12Rect.top = rect.offset.y;
			d3d12Rect.right = rect.offset.x + rect.extent.width;
			d3d12Rect.bottom = rect.offset.y + rect.extent.height;
		}

		m_commandListData.commandList->RSSetScissorRects(static_cast<uint32_t>(d3d12Rects.size()), d3d12Rects.data());
	}

	void D3D12CommandBuffer::BindPipeline(RawPtr<RenderPipeline> pipeline)
	{
		VT_ENSURE(pipeline);

		ClearActivePipeline();
		m_activeRenderPipeline = pipeline;

		D3D12RenderPipeline& d3d12RenderPipeline = pipeline->AsRef<D3D12RenderPipeline>();

		m_commandListData.commandList->SetGraphicsRootSignature(d3d12RenderPipeline.GetRootSignature().rootSignature.Get());
		m_commandListData.commandList->SetPipelineState(d3d12RenderPipeline.GetHandle<ID3D12PipelineState*>());

		D3D12_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		switch (d3d12RenderPipeline.GetTopology())
		{
			case Topology::TriangleList: topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
			case Topology::LineList: topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
			case Topology::TriangleStrip: topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
			case Topology::PointList: topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
			default: VT_ENSURE(false);
		}

		m_commandListData.commandList->IASetPrimitiveTopology(topology);
	}

	void D3D12CommandBuffer::BindPipeline(RawPtr<ComputePipeline> pipeline)
	{
		VT_ENSURE(pipeline);
	
		ClearActivePipeline();
		m_activeComputePipeline = pipeline;

		D3D12ComputePipeline& d3d12ComputePipeline = pipeline->AsRef<D3D12ComputePipeline>();

		m_commandListData.commandList->SetComputeRootSignature(d3d12ComputePipeline.GetRootSignature().rootSignature.Get());
		m_commandListData.commandList->SetPipelineState(d3d12ComputePipeline.GetHandle<ID3D12PipelineState*>());
	}

	void D3D12CommandBuffer::BindPipeline(RawPtr<RayTracingPipeline> pipeline)
	{
		VT_ENSURE(pipeline);

		ClearActivePipeline();
		m_activeRayTracingPipeline = pipeline;

		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::BindVertexBuffers(const VertexBufferVector& vertexBuffers, const uint32_t firstBinding)
	{
		InlineVector<D3D12_VERTEX_BUFFER_VIEW, MAX_VERTEX_BUFFER_COUNT> vertexBufferViews;

		for (const auto& vertexBufferBinding : vertexBuffers)
		{
			auto& newView = vertexBufferViews.emplace_back();
			newView.BufferLocation = vertexBufferBinding.buffer->GetDeviceAddress() + vertexBufferBinding.offset;
			newView.SizeInBytes = static_cast<uint32_t>(vertexBufferBinding.buffer->GetByteSize());
			newView.StrideInBytes = static_cast<uint32_t>(vertexBufferBinding.buffer->GetElementSize());
		}

		m_commandListData.commandList->IASetVertexBuffers(firstBinding, static_cast<uint32_t>(vertexBufferViews.size()), vertexBufferViews.data());
	}

	void D3D12CommandBuffer::BindIndexBuffer(RawPtr<StorageBuffer> indexBuffer, const IndexType indexType)
	{
		D3D12_INDEX_BUFFER_VIEW view{};
		view.BufferLocation = indexBuffer->GetDeviceAddress();
		view.Format = indexType == IndexType::UInt32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		view.SizeInBytes = static_cast<uint32_t>(indexBuffer->GetByteSize());

		m_commandListData.commandList->IASetIndexBuffer(&view);
	}

	void D3D12CommandBuffer::BeginRendering(const RenderingInfo& renderingInfo)
	{
		InlineVector<D3D12_CPU_DESCRIPTOR_HANDLE, MAX_COLOR_ATTACHMENT_COUNT> colorAttachmentInfo{};
		D3D12_CPU_DESCRIPTOR_HANDLE dsvView = { 0 };

		for (const auto& colorAtt : renderingInfo.colorAttachments)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE viewHandle = D3D12_CPU_DESCRIPTOR_HANDLE(colorAtt.view->AsRef<D3D12ImageView>().GetRTVDSVDescriptor().GetCPUPointer());

			if (colorAtt.clearMode == ClearMode::Clear)
			{
				m_commandListData.commandList->ClearRenderTargetView(viewHandle, colorAtt.clearColor.float32, 0, nullptr);
			}
			colorAttachmentInfo.emplace_back(viewHandle);
		}

		if (renderingInfo.depthAttachmentInfo.view)
		{
			dsvView = D3D12_CPU_DESCRIPTOR_HANDLE(renderingInfo.depthAttachmentInfo.view->AsRef<D3D12ImageView>().GetRTVDSVDescriptor().GetCPUPointer());

			if (renderingInfo.depthAttachmentInfo.clearMode == ClearMode::Clear)
			{
				m_commandListData.commandList->ClearDepthStencilView(dsvView, D3D12_CLEAR_FLAG_DEPTH, renderingInfo.depthAttachmentInfo.clearColor.float32[0], 0, 0, nullptr);
			}
		}

		m_commandListData.commandList->OMSetRenderTargets(static_cast<uint32_t>(colorAttachmentInfo.size()), colorAttachmentInfo.data(), false, dsvView.ptr == 0 ? nullptr : &dsvView);
	}

	void D3D12CommandBuffer::EndRendering()
	{
	}

	inline void AddGlobalBarrier(const GlobalBarrier& barrierInfo, D3D12_GLOBAL_BARRIER& barrier)
	{
		barrier.SyncBefore = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.srcStage);
		barrier.AccessBefore = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.srcAccess);
		barrier.SyncAfter = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.dstStage);
		barrier.AccessAfter = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.dstAccess);
	}

	inline void AddBufferBarrier(const BufferBarrier& barrierInfo, D3D12_BUFFER_BARRIER& barrier)
	{
		VT_ENSURE(barrierInfo.resource != nullptr);

		if (barrierInfo.srcStage == BarrierStage::Clear)
		{
			barrier.SyncBefore = D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;
			barrier.AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		}
		else
		{
			barrier.SyncBefore = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.srcStage);
			barrier.AccessBefore = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.srcAccess);
		}

		if (barrierInfo.dstStage == BarrierStage::Clear)
		{
			barrier.SyncAfter = D3D12_BARRIER_SYNC_CLEAR_UNORDERED_ACCESS_VIEW;
			barrier.AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
		}
		else
		{
			barrier.SyncAfter = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.dstStage);
			barrier.AccessAfter = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.dstAccess);
		}

		barrier.Offset = 0;
		barrier.Size = barrierInfo.size;
		barrier.pResource = barrierInfo.resource->GetHandle<ID3D12Resource*>();

		GraphicsContext::GetResourceStateTracker()->TransitionResource(barrierInfo.resource, barrierInfo.dstStage, barrierInfo.dstAccess);
	}

	inline void AddImageBarrier(const ImageBarrier& barrierInfo, D3D12_TEXTURE_BARRIER& barrier)
	{
		VT_ENSURE(barrierInfo.resource != nullptr);

		if (barrierInfo.srcStage == BarrierStage::Clear)
		{
			if (barrierInfo.resource->GetType() == ResourceType::Image2D)
			{
				const auto aspectMask = barrierInfo.resource->As<Image>()->GetImageAspect();
				if (EnumValueContainsFlag(aspectMask, ImageAspect::Depth) || EnumValueContainsFlag(aspectMask, ImageAspect::Stencil))
				{
					barrier.SyncBefore = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
				}
				else
				{
					barrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
				}
			}
		}
		else
		{
			barrier.SyncBefore = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.srcStage);
		}

		if (barrierInfo.dstStage == BarrierStage::Clear)
		{
			if (barrierInfo.resource->GetType() == ResourceType::Image2D)
			{
				const auto aspectMask = barrierInfo.resource->As<Image>()->GetImageAspect();
				if (EnumValueContainsFlag(aspectMask, ImageAspect::Depth) || EnumValueContainsFlag(aspectMask, ImageAspect::Stencil))
				{
					barrier.SyncAfter = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
				}
				else
				{
					barrier.SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET;
				}
			}
		}

		barrier.AccessBefore = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.srcAccess);
		barrier.SyncAfter = Utility::GetD3D12BarrierSyncFromBarrierStage(barrierInfo.dstStage);
		barrier.AccessAfter = Utility::GetD3D12BarrierAccessFromBarrierAccess(barrierInfo.dstAccess);
		barrier.LayoutBefore = Utility::GetD3D12BarrierLayoutFromImageLayout(barrierInfo.srcLayout);
		barrier.LayoutAfter = Utility::GetD3D12BarrierLayoutFromImageLayout(barrierInfo.dstLayout);
		barrier.pResource = barrierInfo.resource->GetHandle<ID3D12Resource*>();
		barrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;

		auto levelCount = barrierInfo.subResource.levelCount;
		if (levelCount == ALL_MIPS)
		{
			if (barrierInfo.resource->GetType() == ResourceType::Image2D || 
				barrierInfo.resource->GetType() == ResourceType::Image1D || 
				barrierInfo.resource->GetType() == ResourceType::Image3D)
			{
				levelCount = barrierInfo.resource->As<Image>()->GetMipCount();
			}
		}

		auto layerCount = barrierInfo.subResource.layerCount;
		if (layerCount == ALL_LAYERS)
		{
			if (barrierInfo.resource->GetType() == ResourceType::Image2D ||
				barrierInfo.resource->GetType() == ResourceType::Image1D ||
				barrierInfo.resource->GetType() == ResourceType::Image3D)
			{
				layerCount = barrierInfo.resource->As<Image>()->GetLayerCount();
			}
		}

		barrier.Subresources.NumMipLevels = levelCount;
		barrier.Subresources.NumArraySlices = layerCount;
		barrier.Subresources.FirstArraySlice = barrierInfo.subResource.baseArrayLayer;
		barrier.Subresources.IndexOrFirstMipLevel = barrierInfo.subResource.baseMipLevel;
		barrier.Subresources.FirstPlane = 0;
		barrier.Subresources.NumPlanes = 1;

		GraphicsContext::GetResourceStateTracker()->TransitionResource(barrierInfo.resource, barrierInfo.dstStage, barrierInfo.dstAccess, barrierInfo.dstLayout);
	}

	void D3D12CommandBuffer::ResourceBarrier(const BarrierVector& resourceBarriers)
	{
		using ImageBarrierVector = Vector<D3D12_TEXTURE_BARRIER, InlineAllocator<16>>;
		using BufferBarrierVector = Vector<D3D12_BUFFER_BARRIER, InlineAllocator<16>>;
		using GlobalBarrierVector = Vector<D3D12_GLOBAL_BARRIER, InlineAllocator<16>>;

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

		InlineVector<D3D12_BARRIER_GROUP, 3u> barrierGroups{};

		if (!memoryBarriers.empty())
		{
			auto& group = barrierGroups.emplace_back();
			group.Type = D3D12_BARRIER_TYPE_GLOBAL;
			group.NumBarriers = static_cast<uint32_t>(memoryBarriers.size());
			group.pGlobalBarriers = memoryBarriers.data();
		}

		if (!imageBarriers.empty())
		{
			auto& group = barrierGroups.emplace_back();
			group.Type = D3D12_BARRIER_TYPE_TEXTURE;
			group.NumBarriers = static_cast<uint32_t>(imageBarriers.size());
			group.pTextureBarriers = imageBarriers.data();
		}

		if (!bufferBarriers.empty())
		{
			auto& group = barrierGroups.emplace_back();
			group.Type = D3D12_BARRIER_TYPE_BUFFER;
			group.NumBarriers = static_cast<uint32_t>(bufferBarriers.size());
			group.pBufferBarriers = bufferBarriers.data();
		}

		if (!barrierGroups.empty())
		{
			m_commandListData.commandList->Barrier(static_cast<uint32_t>(barrierGroups.size()), barrierGroups.data());
		}
	}

	void D3D12CommandBuffer::BuildAccelerationStructures(const Vector<AccelerationStructureBuildGeometryInfo>& buildInfos, const Vector<AccelerationStructureBuildRanges>& buildRanges)
	{
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::BeginMarker(std::string_view markerLabel, const std::array<float, 4>& markerColor)
	{
		uint32_t color = PIX_COLOR(static_cast<BYTE>(markerColor[0] * 255.f), static_cast<BYTE>(markerColor[1] * 255.f), static_cast<BYTE>(markerColor[2] * 255.f));
		PIXBeginEvent(m_commandListData.commandList.Get(), color, markerLabel.data());
	}

	void D3D12CommandBuffer::EndMarker()
	{
		PIXEndEvent(m_commandListData.commandList.Get());
	}

	const uint32_t D3D12CommandBuffer::BeginTimestamp()
	{
		return 0;
	}

	void D3D12CommandBuffer::EndTimestamp(uint32_t timestampIndex)
	{
	}

	const float D3D12CommandBuffer::GetExecutionTime(uint32_t timestampIndex) const
	{
		return 0.f;
	}

	void D3D12CommandBuffer::ClearBufferView(RawPtr<BufferView> bufferView, const uint32_t clearValue)
	{
		D3D12BufferView& d3d12View = bufferView->AsRef<D3D12BufferView>();
		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();

		uint32_t values[4];
		values[0] = clearValue;
		values[1] = clearValue;
		values[2] = clearValue;
		values[3] = clearValue;

		D3D12DescriptorPointer gpuDescriptor = g_descriptorManager.AllocateOnStack(D3D12DescriptorType::CBV_SRV_UAV, 1);
		d3d12Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE(gpuDescriptor.GetCPUPointer()), D3D12_CPU_DESCRIPTOR_HANDLE(d3d12View.GetUAVDescriptor().GetCPUPointer()), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_commandListData.commandList->ClearUnorderedAccessViewUint(
			D3D12_GPU_DESCRIPTOR_HANDLE(gpuDescriptor.GetGPUPointer()),
			D3D12_CPU_DESCRIPTOR_HANDLE(d3d12View.GetUAVDescriptor().GetCPUPointer()),
			bufferView->GetHandle<ID3D12Resource*>(),
			values,
			0, nullptr);
	}

	void D3D12CommandBuffer::ClearBufferView(RawPtr<BufferView> bufferView, const float clearValue)
	{
		D3D12BufferView& d3d12View = bufferView->AsRef<D3D12BufferView>();
		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();

		float values[4];
		values[0] = clearValue;
		values[1] = clearValue;
		values[2] = clearValue;
		values[3] = clearValue;

		D3D12DescriptorPointer gpuDescriptor = g_descriptorManager.AllocateOnStack(D3D12DescriptorType::CBV_SRV_UAV, 1);
		d3d12Device->CopyDescriptorsSimple(1, D3D12_CPU_DESCRIPTOR_HANDLE(gpuDescriptor.GetCPUPointer()), D3D12_CPU_DESCRIPTOR_HANDLE(d3d12View.GetUAVDescriptor().GetCPUPointer()), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_commandListData.commandList->ClearUnorderedAccessViewFloat(
			D3D12_GPU_DESCRIPTOR_HANDLE(gpuDescriptor.GetGPUPointer()),
			D3D12_CPU_DESCRIPTOR_HANDLE(d3d12View.GetUAVDescriptor().GetCPUPointer()),
			bufferView->GetHandle<ID3D12Resource*>(),
			values,
			0, nullptr);
	}

	void D3D12CommandBuffer::ClearImageView(RawPtr<ImageView> imageView, std::array<uint32_t, 4> clearValue)
	{
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::ClearImageView(RawPtr<ImageView> imageView, std::array<float, 4> clearValue)
	{
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::CopyBufferRegion(Handle<Allocation> srcResource, const size_t srcOffset, Handle<Allocation> dstResource, const size_t dstOffset, const size_t size)
	{
		m_commandListData.commandList->CopyBufferRegion(dstResource->GetResourceHandle<ID3D12Resource*>(), dstOffset, srcResource->GetResourceHandle<ID3D12Resource*>(), srcOffset, size);
	}

	void D3D12CommandBuffer::CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip)
	{
		CopyBufferToImage(srcBuffer, dstImage, width, height, depth, 0, 0, 0, mip);
	}

	void D3D12CommandBuffer::CopyBufferToImage(Handle<Allocation> srcBuffer, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth, const int32_t offsetX, const int32_t offsetY, const int32_t offsetZ, const uint32_t mip)
	{
		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");

		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		uint32_t numRows;
		uint64_t rowSizeInBytes;
		uint64_t totalBytes;

		auto device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>();
		ID3D12Device10* d3d12Device = device->GetDevice10();

		const D3D12_RESOURCE_DESC1 resourceDesc = Utility::GetD3D12ResourceDesc(dstImage->GetDesc());
		const uint32_t subResourceIndex = D3D12CalcSubresource(mip, 0, 0, 1, 1);

		d3d12Device->GetCopyableFootprints1(&resourceDesc, subResourceIndex, 1, 0, &footprint, &numRows, &rowSizeInBytes, &totalBytes);

		D3D12_TEXTURE_COPY_LOCATION dst;
		dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst.SubresourceIndex = subResourceIndex;
		dst.pResource = dstImage->GetHandle<ID3D12Resource*>();

		D3D12_TEXTURE_COPY_LOCATION src;
		src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src.PlacedFootprint.Footprint.Format = ConvertFormatToD3D12Format(dstImage->GetFormat());
		src.PlacedFootprint.Footprint.Width = width;
		src.PlacedFootprint.Footprint.Height = height;
		src.PlacedFootprint.Footprint.Depth = 1;
		src.PlacedFootprint.Footprint.RowPitch = static_cast<uint32_t>(rowSizeInBytes);
		src.pResource = srcBuffer->GetResourceHandle<ID3D12Resource*>();

		m_commandListData.commandList->CopyTextureRegion(&dst, offsetX, offsetY, offsetZ, &src, nullptr);
	}

	void D3D12CommandBuffer::CopyImageToBuffer(RawPtr<Image> srcImage, Handle<Allocation> dstBuffer, const size_t dstOffset, const uint32_t width, const uint32_t height, const uint32_t depth, const uint32_t mip)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");
		VT_ENSURE_MSG(mip < srcImage->CalculateMipCount(), "Mip level is not valid!");
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::CopyImage(RawPtr<Image> srcImage, RawPtr<Image> dstImage, const uint32_t width, const uint32_t height, const uint32_t depth)
	{
		VT_PROFILE_FUNCTION();

		VT_ENSURE_MSG(height >= 1 && width >= 1 && depth >= 1, "All dimensions must be equal to or greater than one!");
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::UploadTextureData(RawPtr<Image> dstImage, Handle<Allocation> stagingAllocation, const ImageCopyData& copyData)
	{
		ID3D12Resource* d3d12Image = dstImage->GetHandle<ID3D12Resource*>();
		const D3D12_RESOURCE_DESC desc = d3d12Image->GetDesc();

		const uint32_t mipLevels = static_cast<uint32_t>(desc.MipLevels);
		const uint32_t arraySize = desc.DepthOrArraySize;

		Vector<D3D12_SUBRESOURCE_DATA> subResources;
		subResources.resize(mipLevels * arraySize);

		for (const auto& subData : copyData.copySubData)
		{
			const uint32_t baseMip = subData.subResource.baseMipLevel;
			const uint32_t numMips = subData.subResource.levelCount == ALL_MIPS ? mipLevels - baseMip : subData.subResource.levelCount;
		
			const uint32_t baseArrayLayer = subData.subResource.baseArrayLayer;
			const uint32_t numLayers = subData.subResource.layerCount == ALL_LAYERS ? arraySize - baseArrayLayer : subData.subResource.layerCount;
		
			VT_UNUSED(numLayers);
			VT_UNUSED(numMips);

			const uint8_t* layerDataPtr = static_cast<const uint8_t*>(subData.data);

			for (uint32_t layer = baseArrayLayer; layer < baseArrayLayer + numLayers; ++layer)
			{
				for (uint32_t mip = baseMip; mip < baseMip + numMips; ++mip)
				{
					uint32_t subResourceIndex = D3D12CalcSubresource(mip, layer, 0, mipLevels, arraySize);

					const uint32_t d3d12SlicePitch = subData.rowPitch * subData.height;

					D3D12_SUBRESOURCE_DATA& newSubresource = subResources[subResourceIndex];
					newSubresource.pData = layerDataPtr + (layer * d3d12SlicePitch * subData.depth);
					newSubresource.RowPitch = subData.rowPitch;
					newSubresource.SlicePitch = d3d12SlicePitch;
				}
			}
		}

		const uint32_t numSubresources = static_cast<uint32_t>(subResources.size());
		VT_ENSURE(UpdateSubresources(m_commandListData.commandList.Get(), d3d12Image, stagingAllocation->GetResourceHandle<ID3D12Resource*>(), 0, 0, numSubresources, subResources.data()) > 0);
	}

	const QueueType D3D12CommandBuffer::GetQueueType() const
	{
		return m_queueType;
	}

	const CommandBufferLevel D3D12CommandBuffer::GetCommandBufferLevel() const
	{
		return m_commandBufferLevel;
	}

	RefPtr<CommandBuffer> D3D12CommandBuffer::CreateSecondaryCommandBuffer() const
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);
		return RefPtr<D3D12CommandBuffer>::Create(this);
	}

	void D3D12CommandBuffer::ExecuteSecondaryCommandBuffer(RefPtr<CommandBuffer> commandBuffer) const
	{
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);
		VT_ENSURE(commandBuffer->GetCommandBufferLevel() == CommandBufferLevel::Secondary);
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::ExecuteSecondaryCommandBuffers(Vector<RefPtr<CommandBuffer>> commandBuffers) const
	{
		VT_ENSURE(m_commandBufferLevel == CommandBufferLevel::Primary);
		VT_ENSURE(false);
	}

	void D3D12CommandBuffer::BindShaderBindings(const ShaderBindingMap& shaderBindingsMap)
	{
		VT_PROFILE_FUNCTION();
	
		const auto& shaderBindings = shaderBindingsMap.GetBindings();
		const RootSignatureBuilder::RootSignature* rootSignature = nullptr;

		if (m_activeComputePipeline)
		{
			D3D12ComputePipeline& d3d12ComputePipeline = m_activeComputePipeline->AsRef<D3D12ComputePipeline>();
			rootSignature = &d3d12ComputePipeline.GetRootSignature();
		}
		else if (m_activeRenderPipeline)
		{
			D3D12RenderPipeline& d3d12RenderPipeline = m_activeRenderPipeline->AsRef<D3D12RenderPipeline>();
			rootSignature = &d3d12RenderPipeline.GetRootSignature();
		}

		VT_ENSURE(rootSignature != nullptr);

		InlineVector<D3D12_CPU_DESCRIPTOR_HANDLE, ShaderBindingMap::NumMaxBindings> srcDescriptors;
		InlineVector<D3D12_CPU_DESCRIPTOR_HANDLE, ShaderBindingMap::NumMaxBindings> srcSamplerDescriptors;

		InlineVector<D3D12_CPU_DESCRIPTOR_HANDLE, ShaderBindingMap::NumMaxBindings> dstDescriptors;
		InlineVector<D3D12_CPU_DESCRIPTOR_HANDLE, ShaderBindingMap::NumMaxBindings> dstSamplerDescriptors;

		Array<D3D12DescriptorPointer, GetNumShaderStages()> perShaderStageBaseDescriptor;
		Array<D3D12DescriptorPointer, GetNumShaderStages()> perShaderStageSamplerBaseDescriptor;

		// Unfortunately we need to make a special case here, as D3D12 doesn't allow offsets
		// of views.
		struct OffsetCBVDescriptor
		{
			uint64_t deviceAddress = 0;
		};

		Array<OffsetCBVDescriptor, GetNumShaderStages()> perShaderStageOffsetCBVDescriptors;

		uint32_t descriptorBaseOffset = 0;
		uint32_t samplerDescriptorBaseOffset = 0;

		for (const auto& [shaderStage, bindings] : shaderBindings)
		{
			uint32_t numSamplerDescriptors = 0;
			uint32_t numMainDescriptors = 0;

			for (const ShaderBindingMap::ResourceBinding& binding : bindings)
			{
				// Special case for offset uniform buffers
				if ((binding.uniformBufferOffset > 0 || binding.uniformBufferSize > 0) && binding.registerType == ShaderRegisterType::CBV)
				{
					D3D12BufferView& d3d12BufferView = binding.bufferView->AsRef<D3D12BufferView>();
					perShaderStageOffsetCBVDescriptors[GetDescriptorSetIndexFromShaderStage(shaderStage)].deviceAddress = d3d12BufferView.GetDeviceAddress() + binding.uniformBufferOffset;
					continue;
				}

				uint32_t descriptorIndex = 0;

				if (binding.registerType != ShaderRegisterType::Sampler)
				{
					descriptorIndex = rootSignature->GetFlatDescriptorIndexFromBindingAndType(shaderStage, binding.registerType, binding.bindingIndex);
					srcDescriptors.resize_uninitialized(std::max(descriptorBaseOffset + descriptorIndex + 1, static_cast<uint32_t>(srcDescriptors.size())));
					numMainDescriptors++;
				}
				else
				{
					descriptorIndex = rootSignature->GetFlatSamplerDescriptorIndexFromBinding(shaderStage, binding.bindingIndex);
					srcSamplerDescriptors.resize_uninitialized(std::max(samplerDescriptorBaseOffset + descriptorIndex + 1, static_cast<uint32_t>(srcSamplerDescriptors.size())));
					numSamplerDescriptors++;
				}

				switch (binding.registerType)
				{
					case ShaderRegisterType::CBV:
					{
						D3D12BufferView& d3d12BufferView = binding.bufferView->AsRef<D3D12BufferView>();
						srcDescriptors[descriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12BufferView.GetCBVDescriptor().GetCPUPointer());
						break;
					}

					case ShaderRegisterType::SRV:
					{
						switch (binding.resourceType)
						{
							case ShaderResourceType::StructuredBuffer:
							case ShaderResourceType::TexelBuffer:
							{
								D3D12BufferView& d3d12BufferView = binding.bufferView->AsRef<D3D12BufferView>();
								srcDescriptors[descriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12BufferView.GetSRVDescriptor().GetCPUPointer());
								break;
							}

							case ShaderResourceType::Texture:
							{
								D3D12ImageView& d3d12ImageView = binding.imageView->AsRef<D3D12ImageView>();
								srcDescriptors[descriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12ImageView.GetSRVDescriptor().GetCPUPointer());
								break;
							}

							case ShaderResourceType::AccelerationStructure:
							{
								break;
							}
						}

						break;
					}

					case ShaderRegisterType::UAV:
					{
						switch (binding.resourceType)
						{
							case ShaderResourceType::StructuredBuffer:
							case ShaderResourceType::TexelBuffer:
							{
								D3D12BufferView& d3d12BufferView = binding.bufferView->AsRef<D3D12BufferView>();
								srcDescriptors[descriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12BufferView.GetUAVDescriptor().GetCPUPointer());
								break;
							}

							case ShaderResourceType::Texture:
							{
								D3D12ImageView& d3d12ImageView = binding.imageView->AsRef<D3D12ImageView>();
								srcDescriptors[descriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12ImageView.GetUAVDescriptor().GetCPUPointer());
								break;
							}
						}

						break;
					}

					case ShaderRegisterType::Sampler:
					{
						D3D12SamplerState& d3d12SamplerState = binding.samplerState->AsRef<D3D12SamplerState>();
						srcSamplerDescriptors[samplerDescriptorBaseOffset + descriptorIndex] = D3D12_CPU_DESCRIPTOR_HANDLE(d3d12SamplerState.GetDescriptor().GetCPUPointer());
						break;
					}
				}
			}

			descriptorBaseOffset += numMainDescriptors;
			samplerDescriptorBaseOffset += numSamplerDescriptors;
			
			if (numMainDescriptors > 0)
			{
				size_t startOffset = dstDescriptors.size();
				dstDescriptors.resize_uninitialized(dstDescriptors.size() + numMainDescriptors);

				D3D12DescriptorPointer baseDescriptor = g_descriptorManager.AllocateOnStack(D3D12DescriptorType::CBV_SRV_UAV, numMainDescriptors);
				const uint64_t descriptorSize = g_descriptorManager.GetDescriptorSize(D3D12DescriptorType::CBV_SRV_UAV);

				perShaderStageBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)] = baseDescriptor;

				for (size_t i = 0; i < numMainDescriptors; ++i)
				{
					dstDescriptors[startOffset + i] = D3D12_CPU_DESCRIPTOR_HANDLE(baseDescriptor.GetCPUPointer() + i * descriptorSize);
				}
			}

			if (numSamplerDescriptors > 0)
			{
				size_t startOffset = dstSamplerDescriptors.size();
				dstSamplerDescriptors.resize_uninitialized(dstSamplerDescriptors.size() + numSamplerDescriptors);

				D3D12DescriptorPointer baseDescriptor = g_descriptorManager.AllocateOnStack(D3D12DescriptorType::Sampler, numSamplerDescriptors);
				const uint64_t descriptorSize = g_descriptorManager.GetDescriptorSize(D3D12DescriptorType::Sampler);

				perShaderStageSamplerBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)] = baseDescriptor;

				for (size_t i = 0; i < numSamplerDescriptors; ++i)
				{
					dstSamplerDescriptors[startOffset + i] = D3D12_CPU_DESCRIPTOR_HANDLE(baseDescriptor.GetCPUPointer() + i * descriptorSize);
				}
			}
		}

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>()->GetDevice10();

		InlineVector<uint32_t, ShaderBindingMap::NumMaxBindings> copyRangeSizes;

		if (!srcDescriptors.empty())
		{
			copyRangeSizes.resize_uninitialized(srcDescriptors.size());
			std::fill(copyRangeSizes.begin(), copyRangeSizes.end(), 1u);

			d3d12Device->CopyDescriptors(
				static_cast<uint32_t>(copyRangeSizes.size()),
				dstDescriptors.data(),
				copyRangeSizes.data(),
				static_cast<uint32_t>(copyRangeSizes.size()),
				srcDescriptors.data(),
				copyRangeSizes.data(),
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		}

		if (!srcSamplerDescriptors.empty())
		{
			copyRangeSizes.resize_uninitialized(srcSamplerDescriptors.size());
			std::fill(copyRangeSizes.begin(), copyRangeSizes.end(), 1u);

			d3d12Device->CopyDescriptors(
				static_cast<uint32_t>(copyRangeSizes.size()),
				dstSamplerDescriptors.data(),
				copyRangeSizes.data(),
				static_cast<uint32_t>(copyRangeSizes.size()),
				srcSamplerDescriptors.data(),
				copyRangeSizes.data(),
				D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
			);
		}

		if (m_activeComputePipeline)
		{
			for (const auto& [shaderStage, bindings] : shaderBindings)
			{
				const D3D12DescriptorPointer& descriptorPointer = perShaderStageBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (descriptorPointer.IsValid())
				{
					m_commandListData.commandList->SetComputeRootDescriptorTable(rootSignature->GetDescriptorTableIndexFromShaderStage(shaderStage), D3D12_GPU_DESCRIPTOR_HANDLE(descriptorPointer.GetGPUPointer()));
				}

				const D3D12DescriptorPointer& samplerDescriptorPointer = perShaderStageSamplerBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (samplerDescriptorPointer.IsValid())
				{
					m_commandListData.commandList->SetComputeRootDescriptorTable(rootSignature->GetSamplerDescriptorTableIndexFromShaderStage(shaderStage), D3D12_GPU_DESCRIPTOR_HANDLE(samplerDescriptorPointer.GetGPUPointer()));
				}

				const OffsetCBVDescriptor& offsetCBVDescriptor = perShaderStageOffsetCBVDescriptors[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (offsetCBVDescriptor.deviceAddress != 0)
				{
					m_commandListData.commandList->SetComputeRootConstantBufferView(rootSignature->GetGlobalsRootIndexFromShaderStage(shaderStage), D3D12_GPU_VIRTUAL_ADDRESS(offsetCBVDescriptor.deviceAddress));
				}
			}
		}
		else if (m_activeRenderPipeline)
		{
			for (const auto& [shaderStage, bindings] : shaderBindings)
			{
				const D3D12DescriptorPointer& descriptorPointer = perShaderStageBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (descriptorPointer.IsValid())
				{
					m_commandListData.commandList->SetGraphicsRootDescriptorTable(rootSignature->GetDescriptorTableIndexFromShaderStage(shaderStage), D3D12_GPU_DESCRIPTOR_HANDLE(descriptorPointer.GetGPUPointer()));
				}

				const D3D12DescriptorPointer& samplerDescriptorPointer = perShaderStageSamplerBaseDescriptor[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (samplerDescriptorPointer.IsValid())
				{
					m_commandListData.commandList->SetGraphicsRootDescriptorTable(rootSignature->GetSamplerDescriptorTableIndexFromShaderStage(shaderStage), D3D12_GPU_DESCRIPTOR_HANDLE(samplerDescriptorPointer.GetGPUPointer()));
				}

				const OffsetCBVDescriptor& offsetCBVDescriptor = perShaderStageOffsetCBVDescriptors[GetDescriptorSetIndexFromShaderStage(shaderStage)];
				if (offsetCBVDescriptor.deviceAddress != 0)
				{
					m_commandListData.commandList->SetGraphicsRootConstantBufferView(rootSignature->GetGlobalsRootIndexFromShaderStage(shaderStage), D3D12_GPU_VIRTUAL_ADDRESS(offsetCBVDescriptor.deviceAddress));
				}
			}
		}
		else
		{
			VT_ENSURE(false);
		}
	}

	void* D3D12CommandBuffer::GetHandleImpl() const
	{
		return m_commandListData.commandList.Get();
	}

	void D3D12CommandBuffer::Invalidate()
	{
		VT_PROFILE_FUNCTION();

		auto device = GraphicsContext::GetDevice()->As<D3D12GraphicsDevice>();
		ID3D12Device10* d3d12Device = device->GetDevice10();

		std::wstring commandListName = L"CommandList - ";
		D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_NONE;

		switch (m_queueType)
		{
			case QueueType::Graphics:
				commandListName += L"Graphics";
				commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
				break;
			case QueueType::Compute:
				commandListName += L"Compute";
				commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
				break;
			case QueueType::TransferCopy:
				commandListName += L"TransferCopy";
				commandListType = D3D12_COMMAND_LIST_TYPE_COPY;
				break;
		}

		VT_ENSURE_MSG(commandListType != D3D12_COMMAND_LIST_TYPE_NONE, "Invalid command list type!");
	
		VT_D3D12_CHECK(d3d12Device->CreateCommandAllocator(commandListType, VT_D3D12_ID(m_commandListData.commandAllocator)));
		VT_D3D12_CHECK(d3d12Device->CreateCommandList(0, commandListType, m_commandListData.commandAllocator.Get(), nullptr, VT_D3D12_ID(m_commandListData.commandList)));
		m_commandListData.commandList->Close();
		m_commandListData.commandList->SetName(commandListName.c_str());
	}

	void D3D12CommandBuffer::Release()
	{
		VT_PROFILE_FUNCTION();

		if (!m_commandListData.commandList)
		{
			return;
		}

		RHIModule::GetInstance().DestroyResource([commandAllocator = m_commandListData.commandAllocator, commandList = m_commandListData.commandList, submissionFence = m_submissionFence]() mutable
		{
			if (submissionFence)
			{
				submissionFence->WaitUntilSignaled();
			}

			// Not really needed, just for clarity.
			commandList.Reset();
			commandAllocator.Reset();
		});
	}

	void D3D12CommandBuffer::ClearActivePipeline()
	{
		m_activeRayTracingPipeline.Reset();
		m_activeComputePipeline.Reset();
		m_activeRenderPipeline.Reset();
	}

	bool D3D12CommandBuffer::HasFinishedExecution() const
	{
		if (m_submissionFence)
		{
			return m_submissionFence->IsSignaled();
		}

		return true;
	}

	void D3D12CommandBuffer::BindDescriptorHeaps()
	{
		ID3D12DescriptorHeap* heaps[2]{};
		heaps[0] = g_descriptorManager.GetMainDescriptorStack().GetDescriptorHeap().Get();
		heaps[1] = g_descriptorManager.GetSamplerDescriptorStack().GetDescriptorHeap().Get();

		m_commandListData.commandList->SetDescriptorHeaps(2, heaps);
	}
}
