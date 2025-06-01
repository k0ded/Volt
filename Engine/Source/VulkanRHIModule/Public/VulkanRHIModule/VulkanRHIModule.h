#pragma once

#include "VulkanRHIModule/Core.h"

#include <RHIModule/RHIModule.h>
#include <RHIModule/ResourceDeletionQueue.h>

namespace Volt::RHI
{
	class VulkanRHIModule : public RHIModule
	{
	public:
		VulkanRHIModule();

		RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification) const override;

		RefPtr<CommandBuffer> CreateCommandBuffer(QueueType queueType) const override;

		RefPtr<IndexBuffer> CreateIndexBuffer(std::span<const uint32_t> indices) const override;
		RefPtr<VertexBuffer> CreateVertexBuffer(const void* data, const uint32_t size, const uint32_t stride) const override;

		RefPtr<StorageBuffer> CreateStorageBuffer(uint32_t count, uint64_t elementSize, const std::string& name, BufferUsage bufferUsage, MemoryUsage memoryUsage, RefPtr<GPUAllocator> allocator) const override;
		RefPtr<UniformBuffer> CreateUniformBuffer(const uint32_t size, const void* data, const uint32_t count, const std::string& name) const override;

		RefPtr<DescriptorTable> CreateDescriptorTable(const DescriptorTableCreateInfo& createInfo) const override;
		RefPtr<BindlessDescriptorTable> CreateBindlessDescriptorTable(const uint64_t framesInFlight) const override;

		RefPtr<DeviceQueue> CreateDeviceQueue(const DeviceQueueCreateInfo& createInfo) const override;
		RefPtr<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const override;
		RefPtr<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo) const override;
		RefPtr<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo) const override;
		RefPtr<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const override;

		RefPtr<Image> CreateImage(const ImageSpecification& specification, const void* data, RefPtr<GPUAllocator> allocator) const override;
		RefPtr<Image> CreateImage(const SwapchainImageSpecification& specification) const override;

		RefPtr<ImageView> CreateImageView(const ImageViewDesc& specification) const override;
		RefPtr<SamplerState> CreateSamplerState(const SamplerStateCreateInfo& createInfo) const override;

		RefPtr<DefaultGPUAllocator> CreateDefaultAllocator() const override;
		RefPtr<TransientGPUAllocator> CreateTransientAllocator() const override;
		RefPtr<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const override;

		RefPtr<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const override;
		RefPtr<ComputePipeline> CreateComputePipeline(RefPtr<Shader> shader) const override;
		RefPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const override;

		RefPtr<Shader> CreateShader(const ShaderCreateInfo& specification) const override;
		RefPtr<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const override;

		RefPtr<Event> CreateEvent(const EventCreateInfo& createInfo) const override;
		RefPtr<Fence> CreateFence(const FenceCreateInfo& createInfo) const override;
		RefPtr<Semaphore> CreateSemaphore(const SemaphoreCreateInfo& createInfo) const override;
	
		RefPtr<ImGuiImplementation> CreateImGuiImplementation(const ImGuiCreateInfo& createInfo) const override;

		RefPtr<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const override;
		RefPtr<ShaderBindingTable> CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const override;

		void SetRHICallbackInfo(const RHICallbackInfo& callbackInfo) override;
		void DestroyResource(std::function<void()>&& function) override;
		void RequestApplicationClose() override;
		void Update() override;
		void FlushResourceDeletionQueue() override;

	private:
		RHICallbackInfo m_callbackInfo;
		ResourceDeletionQueue m_resourceDeletionQueue;
		uint32_t m_frameIndex = 0;
	};
}

extern "C"
{
	VTVK_API Volt::RHI::RHIModule* CreateRHIModule();
	VTVK_API void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule);
}
