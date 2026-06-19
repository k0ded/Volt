#pragma once

#include "VulkanRHIModule/Core.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"

#include "VulkanRHIModule/Buffers/VulkanBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanTransientBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"
#include "VulkanRHIModule/Images/VulkanImage.h"
#include "VulkanRHIModule/Images/VulkanTransientImage.h"
#include "VulkanRHIModule/Images/VulkanSamplerState.h"

#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"
#include "VulkanRHIModule/Synchronization/VulkanSemaphore.h"
#include "VulkanRHIModule/VulkanRHISubmissionThread.h"

#include <RHIModule/RHIModule.h>
#include <RHIModule/ResourceDeletionQueue.h>

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace Volt::RHI
{
	class VulkanCPUAllocator;
	class VulkanRHIModule : public RHIModule
	{
	public:
		VulkanRHIModule();
		~VulkanRHIModule() override;

		IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<Buffer> buffer) const override;
		IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const override;

		IntRef<CommandBuffer> CreateCommandBuffer(QueueType queueType) const override;
		IntRef<CommandBuffer> CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const override;

		IntRef<Buffer> CreateBuffer(const BufferDesc& desc) const override;
		IntRef<TransientBuffer> CreateTransientBuffer(const BufferDesc& desc) const override;
		IntRef<UniformBuffer> CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const override;

		IntRef<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const override;
		IntRef<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const override;
		IntRef<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const override;
		IntRef<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const override;

		IntRef<Image> CreateImage(const ImageDesc& specification, const void* data) const override;
		IntRef<Image> CreateImage(const SwapchainImageDesc& specification) const override;
		IntRef<TransientImage> CreateTransientImage(const ImageDesc& desc) const override;

		IntRef<ImageView> CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const override;
		IntRef<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const override;

		IntRef<DefaultGPUAllocator> CreateDefaultAllocator() const override;
		IntRef<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const override;

		IntRef<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const override;
		IntRef<ComputePipeline> CreateComputePipeline(IntRef<Shader> shader) const override;
		IntRef<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const override;

		IntRef<Shader> CreateShader(const ShaderCreateInfo& specification) const override;
		IntRef<Shader> CreateShaderWithSource(const ShaderCreateInfo& specification, const String& source) const override;
		IntRef<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const override;

		IntRef<Fence> CreateFence() const override;
		IntRef<Semaphore> CreateSemaphore() const override;
	
		IntRef<ResourceTable> CreateResourceTable() const override;

		IntRef<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const override;
		IntRef<ShaderBindingTable> CreateShaderBindingTable(IntRef<RayTracingPipeline> pipeline) const override;

		void SetRHICallbackInfo(const RHICallbackInfo& callbackInfo) override;
		void DestroyResource(std::function<void()>&& function, IntRef<Fence> waitForFence) override;
		void RequestApplicationClose() override;
		void BeginFrame() override;
		void EndFrame() override;
		void Shutdown() override;
		void FlushResourceDeletionQueue() override;

	protected:
		RHISubmissionThread& GetSubmissionThreadInternal() override { return m_submissionThread; }

	private:
		RHICallbackInfo m_callbackInfo;
		ResourceDeletionQueue m_resourceDeletionQueue;
		uint32_t m_frameIndex = 0;

		// Arenas
		mutable PagedAtomicArenaAllocator<VulkanBufferView, 1024> m_bufferViewArena;
		mutable PagedAtomicArenaAllocator<VulkanImageView, 1024> m_imageViewArena;

		mutable PagedAtomicArenaAllocator<VulkanBuffer, 1024> m_bufferArena;
		mutable PagedAtomicArenaAllocator<VulkanTransientBuffer, 1024> m_transientBufferArena;
		mutable PagedAtomicArenaAllocator<VulkanUniformBuffer, 1024> m_uniformBufferArena;
		mutable PagedAtomicArenaAllocator<VulkanImage, 1024> m_imageArena;
		mutable PagedAtomicArenaAllocator<VulkanTransientImage, 1024> m_transientImageArena;
		mutable PagedAtomicArenaAllocator<VulkanSamplerState, 1024> m_samplerStateArena;

		mutable PagedAtomicArenaAllocator<VulkanFence, 1024> m_fenceArena;
		mutable PagedAtomicArenaAllocator<VulkanSemaphore, 1024> m_semaphoreArena;
		mutable PagedAtomicArenaAllocator<VulkanCommandBuffer, 1024> m_commandBufferArena;

		Ref<VulkanCPUAllocator> m_vulkanCpuAllocator;
		VulkanRHISubmissionThread m_submissionThread;
	};
}

extern "C"
{
	VTVK_API Volt::RHI::RHIModule* CreateRHIModule();
	VTVK_API void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule);
}
