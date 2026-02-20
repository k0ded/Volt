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

		RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<Buffer> buffer) const override;
		RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const override;

		RefPtr<CommandBuffer> CreateCommandBuffer(QueueType queueType) const override;
		RefPtr<CommandBuffer> CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const override;

		RefPtr<Buffer> CreateBuffer(const BufferDesc& desc) const override;
		RefPtr<TransientBuffer> CreateTransientBuffer(const BufferDesc& desc) const override;
		RefPtr<UniformBuffer> CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const override;

		RefPtr<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const override;
		RefPtr<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const override;
		RefPtr<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const override;
		RefPtr<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const override;

		RefPtr<Image> CreateImage(const ImageDesc& specification, const void* data) const override;
		RefPtr<Image> CreateImage(const SwapchainImageDesc& specification) const override;
		RefPtr<TransientImage> CreateTransientImage(const ImageDesc& desc) const override;

		RefPtr<ImageView> CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const override;
		RefPtr<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const override;

		RefPtr<DefaultGPUAllocator> CreateDefaultAllocator() const override;
		RefPtr<TransientGPUAllocator> CreateTransientAllocator() const override;
		RefPtr<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const override;

		RefPtr<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const override;
		RefPtr<ComputePipeline> CreateComputePipeline(RefPtr<Shader> shader) const override;
		RefPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const override;

		RefPtr<Shader> CreateShader(const ShaderCreateInfo& specification) const override;
		RefPtr<Shader> CreateShaderWithSource(const ShaderCreateInfo& specification, const std::string& source) const override;
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
		mutable PagedAtomicArenaAllocator<VulkanBufferView, 1024> m_bufferViewArena;
		mutable PagedAtomicArenaAllocator<VulkanImageView, 1024> m_imageViewArena;

		mutable PagedAtomicArenaAllocator<VulkanBuffer, 1024> m_bufferArena;
		mutable PagedAtomicArenaAllocator<VulkanTransientBuffer, 1024> m_transientBufferArena;
		mutable PagedAtomicArenaAllocator<VulkanUniformBuffer, 1024> m_uniformBufferArena;
		mutable PagedAtomicArenaAllocator<VulkanImage, 1024> m_imageArena;
		mutable PagedAtomicArenaAllocator<VulkanTransientImage, 1024> m_transientImageArena;
		mutable PagedAtomicArenaAllocator<VulkanSamplerState, 1024> m_samplerStateArena;

		Ref<VulkanCPUAllocator> m_vulkanCpuAllocator;
	};
}

extern "C"
{
	VTVK_API Volt::RHI::RHIModule* CreateRHIModule();
	VTVK_API void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule);
}
