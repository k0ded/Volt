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

namespace Volt::RHI
{
	class D3D12RHIModule : public RHIModule
	{
	public:
		D3D12RHIModule();
		~D3D12RHIModule() override = default;

		RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const override;
		RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const override;

		RefPtr<CommandBuffer> CreateCommandBuffer(QueueType queueType) const override;

		RefPtr<StorageBuffer> CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const override;
		RefPtr<UniformBuffer> CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const override;

		RefPtr<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const override;
		RefPtr<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const override;
		RefPtr<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const override;
		RefPtr<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const override;

		RefPtr<Image> CreateImage(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator) const override;
		RefPtr<Image> CreateImage(const SwapchainImageDesc& specification) const override;

		RefPtr<ImageView> CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const override;
		RefPtr<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const override;

		RefPtr<DefaultGPUAllocator> CreateDefaultAllocator() const override;
		RefPtr<TransientGPUAllocator> CreateTransientAllocator() const override;
		RefPtr<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const override;

		RefPtr<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const override;
		RefPtr<ComputePipeline> CreateComputePipeline(RefPtr<Shader> shader) const override;
		RefPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const override;

		RefPtr<Shader> CreateShader(const ShaderCreateInfo& specification) const override;
		RefPtr<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const override;

		RefPtr<Fence> CreateFence() const override;

		RefPtr<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const override;
		RefPtr<ShaderBindingTable> CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const override;
		RefPtr<RayTracingResourceTable> CreateRayTracingResourceTable() const override;

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
		mutable FixedSizeArenaAllocator<D3D12BufferView> m_bufferViewArena;
		mutable FixedSizeArenaAllocator<D3D12ImageView> m_imageViewArena;

		mutable FixedSizeArenaAllocator<D3D12StorageBuffer> m_storageBufferArena;
		mutable FixedSizeArenaAllocator<D3D12UniformBuffer> m_uniformBufferArena;
		mutable FixedSizeArenaAllocator<D3D12Image> m_imageArena;
		mutable FixedSizeArenaAllocator<D3D12SamplerState> m_samplerStateArena;
	};
}

extern "C" 
{
	VTDX_API Volt::RHI::RHIModule* CreateRHIModule();
	VTDX_API void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule);
}
