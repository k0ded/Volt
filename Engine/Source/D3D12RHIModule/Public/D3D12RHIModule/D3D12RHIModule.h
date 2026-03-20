#pragma once

#include "D3D12RHIModule/Core.h"

#include "D3D12RHIModule/Buffers/D3D12BufferView.h"
#include "D3D12RHIModule/Buffers/D3D12UniformBuffer.h"
#include "D3D12RHIModule/Buffers/D3D12StorageBuffer.h"
#include "D3D12RHIModule/Images/D3D12Image.h"
#include "D3D12RHIModule/Images/D3D12ImageView.h"
#include "D3D12RHIModule/Images/D3D12SamplerState.h"

#include <RHIModule/RHIModule.h>
#include <RHIModule/ResourceDeletionQueue.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt::RHI
{
	class D3D12RHIModule : public RHIModule
	{
	public:
		D3D12RHIModule();
		~D3D12RHIModule() override = default;

		IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const override;
		IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const override;

		IntRef<CommandBuffer> CreateCommandBuffer(QueueType queueType) const override;
		IntRef<CommandBuffer> CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const override;

		IntRef<StorageBuffer> CreateStorageBuffer(const BufferDesc& desc, IntRef<GPUAllocator> allocator) const override;
		IntRef<UniformBuffer> CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const override;

		IntRef<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const override;
		IntRef<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const override;
		IntRef<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const override;
		IntRef<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const override;

		IntRef<Image> CreateImage(const ImageDesc& specification, const void* data, IntRef<GPUAllocator> allocator) const override;
		IntRef<Image> CreateImage(const SwapchainImageDesc& specification) const override;

		IntRef<ImageView> CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const override;
		IntRef<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const override;

		IntRef<DefaultGPUAllocator> CreateDefaultAllocator() const override;
		IntRef<TransientGPUAllocator> CreateTransientAllocator() const override;
		IntRef<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const override;

		IntRef<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const override;
		IntRef<ComputePipeline> CreateComputePipeline(IntRef<Shader> shader) const override;
		IntRef<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const override;

		IntRef<Shader> CreateShader(const ShaderCreateInfo& specification) const override;
		IntRef<Shader> CreateShaderWithSource(const ShaderCreateInfo& specification, const std::string& source) const override;
		IntRef<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const override;

		IntRef<Fence> CreateFence() const override;

		IntRef<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const override;
		IntRef<ShaderBindingTable> CreateShaderBindingTable(IntRef<RayTracingPipeline> pipeline) const override;
		IntRef<RayTracingResourceTable> CreateRayTracingResourceTable() const override;

		void SetRHICallbackInfo(const RHICallbackInfo& callbackInfo) override;
		void DestroyResource(std::function<void()>&& function) override;
		void RequestApplicationClose() override;
		void BeginFrame() override;
		void EndFrame() override;
		void FlushResourceDeletionQueue() override;

	private:
		RHICallbackInfo m_callbackInfo;
		ResourceDeletionQueue m_resourceDeletionQueue;
		uint32_t m_frameIndex = 0;

		// Arenas
		mutable PagedAtomicArenaAllocator<D3D12BufferView, 1024> m_bufferViewArena;
		mutable PagedAtomicArenaAllocator<D3D12ImageView, 1024> m_imageViewArena;

		mutable PagedAtomicArenaAllocator<D3D12StorageBuffer, 1024> m_storageBufferArena;
		mutable PagedAtomicArenaAllocator<D3D12UniformBuffer, 1024> m_uniformBufferArena;
		mutable PagedAtomicArenaAllocator<D3D12Image, 1024> m_imageArena;
		mutable PagedAtomicArenaAllocator<D3D12SamplerState, 1024> m_samplerStateArena;
	};
}

extern "C" 
{
	VTDX_API Volt::RHI::RHIModule* CreateRHIModule();
	VTDX_API void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule);
}
