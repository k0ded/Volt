#include "vkpch.h"
#include "VulkanRHIModule.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorHeap.h"

#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Images/VulkanSamplerState.h"

#include "VulkanRHIModule/Memory/VulkanDefaultGPUAllocator.h"
#include "VulkanRHIModule/Memory/VulkanTransientHeap.h"

#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanComputePipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"

#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Descriptors/VulkanResourceTable.h"

#include "VulkanRHIModule/RayTracing/VulkanAccelerationStructure.h"
#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"

#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

#include <CoreUtilities/Profiling/Profiling.h>

#include <tracy/TracyVulkan.hpp>

namespace Volt::RHI
{
	VulkanRHIModule::VulkanRHIModule()
	{
		VT_UNUSED(m_frameIndex);

		s_instance = this;

		m_vulkanCpuAllocator = CreateRef<VulkanCPUAllocator>();
	}

	VulkanRHIModule::~VulkanRHIModule()
	{
	}

	IntRef<BufferView> VulkanRHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<Buffer> buffer) const
	{
		IntRef<BufferView> bufferView = IntRef<VulkanBufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	IntRef<BufferView> VulkanRHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const
	{
		IntRef<BufferView> bufferView = IntRef<VulkanBufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	IntRef<CommandBuffer> VulkanRHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		IntRef<VulkanCommandBuffer> commandBuffer = IntRef<VulkanCommandBuffer>::AttachNoRef(m_commandBufferArena.Allocate(queueType));
		commandBuffer->SetArena(&m_commandBufferArena);

		return commandBuffer;
	}

	IntRef<Buffer> VulkanRHIModule::CreateBuffer(const BufferDesc& desc) const
	{
		IntRef<Buffer> storageBuffer = IntRef<VulkanBuffer>::AttachNoRef(m_bufferArena.Allocate(desc));
		storageBuffer->SetArena(&m_bufferArena);
		return storageBuffer;
	}

	IntRef<UniformBuffer> VulkanRHIModule::CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData = nullptr) const
	{
		IntRef<UniformBuffer> uniformBuffer = IntRef<VulkanUniformBuffer>::AttachNoRef(m_uniformBufferArena.Allocate(uniformBufferDesc, initialData));
		uniformBuffer->SetArena(&m_uniformBufferArena);
		return uniformBuffer;
	}

	IntRef<GraphicsContext> VulkanRHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return IntRef<VulkanGraphicsContext>::Create(createInfo);
	}

	IntRef<GraphicsDevice> VulkanRHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const
	{
		return IntRef<VulkanGraphicsDevice>::Create(createInfo, physicalGraphicsDevice, enableDebugLayer);
	}

	IntRef<PhysicalGraphicsDevice> VulkanRHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const
	{
		return IntRef<VulkanPhysicalGraphicsDevice>::Create(createInfo, enableDebugLayer);
	}

	IntRef<Swapchain> VulkanRHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return IntRef<VulkanSwapchain>::Create(createInfo);
	}

	IntRef<Image> VulkanRHIModule::CreateImage(const ImageDesc& specification, const void* data) const
	{
		IntRef<VulkanImage> image = IntRef<VulkanImage>::AttachNoRef(m_imageArena.Allocate(specification, data));
		image->SetArena(&m_imageArena);

		return image;
	}

	IntRef<Image> VulkanRHIModule::CreateImage(const SwapchainImageDesc& specification) const
	{
		IntRef<VulkanImage> image = IntRef<VulkanImage>::AttachNoRef(m_imageArena.Allocate(specification));
		image->SetArena(&m_imageArena);

		return image;
	}

	IntRef<ImageView> VulkanRHIModule::CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const
	{
		IntRef<ImageView> imageView = IntRef<VulkanImageView>::AttachNoRef(m_imageViewArena.Allocate(specification, image));
		imageView->SetArena(&m_imageViewArena);

		return imageView;
	}

	IntRef<SamplerState> VulkanRHIModule::CreateSamplerState(const SamplerStateDesc& createInfo) const
	{
		IntRef<SamplerState> samplerState = IntRef<VulkanSamplerState>::AttachNoRef(m_samplerStateArena.Allocate(createInfo));
		samplerState->SetArena(&m_samplerStateArena);

		return samplerState;
	}

	IntRef<DefaultGPUAllocator> VulkanRHIModule::CreateDefaultAllocator() const
	{
		return IntRef<VulkanDefaultGPUAllocator>::Create();
	}

	IntRef<TransientHeap> VulkanRHIModule::CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const
	{
		return IntRef<VulkanTransientHeap>::Create(createInfo);
	}

	IntRef<Volt::RHI::ComputePipeline> VulkanRHIModule::CreateComputePipeline(IntRef<Shader> shader) const
	{
		return IntRef<VulkanComputePipeline>::Create(shader);
	}

	IntRef<RayTracingPipeline> VulkanRHIModule::CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
	{
		return IntRef<VulkanRayTracingPipeline>::Create(createInfo);
	}

	IntRef<ShaderCompiler> VulkanRHIModule::CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const
	{
		return IntRef<VulkanShaderCompiler>::Create(createInfo);
	}

	IntRef<Fence> VulkanRHIModule::CreateFence() const
	{
		IntRef<VulkanFence> fence = IntRef<VulkanFence>::AttachNoRef(m_fenceArena.Allocate());
		fence->SetArena(&m_fenceArena);

		return fence;
	}

	IntRef<AccelerationStructure> VulkanRHIModule::CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const
	{
		return IntRef<VulkanAccelerationStructure>::Create(createInfo);
	}

	IntRef<ShaderBindingTable> VulkanRHIModule::CreateShaderBindingTable(IntRef<RayTracingPipeline> pipeline) const
	{
		return IntRef<VulkanShaderBindingTable>::Create(pipeline);
	}

	void VulkanRHIModule::SetRHICallbackInfo(const RHICallbackInfo& callbackInfo)
	{
		m_callbackInfo = callbackInfo;
	}

	void VulkanRHIModule::DestroyResource(std::function<void()>&& function, IntRef<Fence> waitForFence)
	{
		m_resourceDeletionQueue.EnqueueResourceDeletion(std::move(function), waitForFence);
	}

	void VulkanRHIModule::RequestApplicationClose()
	{
		if (m_callbackInfo.requestCloseEventCallback)
		{
			m_callbackInfo.requestCloseEventCallback();
		}
	}

	void VulkanRHIModule::BeginFrame()
	{
		VT_PROFILE_FUNCTION();
		TracyVkCollectHost(GraphicsContext::GetDevice()->AsRef<VulkanGraphicsDevice>().GetProfilingContext());

		GraphicsContext::GetDefaultAllocator()->Update();

		VulkanGraphicsContext& vkGraphicsContext = GraphicsContext::Get().AsRef<VulkanGraphicsContext>();
		vkGraphicsContext.GetDescriptorHeap().BeginFrame();

		m_resourceDeletionQueue.FlushQueue();
	}

	void VulkanRHIModule::FlushResourceDeletionQueue()
	{
		m_resourceDeletionQueue.FlushAll();
	}

	IntRef<Shader> VulkanRHIModule::CreateShader(const ShaderCreateInfo& specification) const
	{
		return IntRef<VulkanShader>::Create(specification);
	}

	IntRef<Shader> VulkanRHIModule::CreateShaderWithSource(const ShaderCreateInfo& specification, const String& source) const
	{
		return IntRef<VulkanShader>::Create(specification, source);
	}

	IntRef<RenderPipeline> VulkanRHIModule::CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const
	{
		return IntRef<VulkanRenderPipeline>::Create(createInfo);
	}

	IntRef<ResourceTable> VulkanRHIModule::CreateResourceTable() const
	{
		return IntRef<VulkanResourceTable>::Create();
	}

	void VulkanRHIModule::EndFrame()
	{
		VulkanGraphicsContext& vkGraphicsContext = GraphicsContext::Get().AsRef<VulkanGraphicsContext>();
		vkGraphicsContext.GetDescriptorHeap().Flush();
	}

	IntRef<CommandBuffer> VulkanRHIModule::CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const
	{
		IntRef<VulkanCommandBuffer> commandBuffer = IntRef<VulkanCommandBuffer>::AttachNoRef(m_commandBufferArena.Allocate(renderingAttachmentDeclaration));
		commandBuffer->SetArena(&m_commandBufferArena);

		return commandBuffer;
	}

	IntRef<TransientBuffer> VulkanRHIModule::CreateTransientBuffer(const BufferDesc& desc) const
	{
		IntRef<TransientBuffer> buffer = IntRef<VulkanTransientBuffer>::AttachNoRef(m_transientBufferArena.Allocate(desc));
		buffer->SetArena(&m_transientBufferArena);
		return buffer;
	}

	IntRef<TransientImage> VulkanRHIModule::CreateTransientImage(const ImageDesc& desc) const
	{
		IntRef<VulkanTransientImage> image = IntRef<VulkanTransientImage>::AttachNoRef(m_transientImageArena.Allocate(desc));
		image->SetArena(&m_transientImageArena);
		return image;
	}

	IntRef<Semaphore> VulkanRHIModule::CreateSemaphore() const
	{
		IntRef<VulkanSemaphore> semaphore = IntRef<VulkanSemaphore>::AttachNoRef(m_semaphoreArena.Allocate());
		semaphore->SetArena(&m_semaphoreArena);

		return semaphore;
	}

	void VulkanRHIModule::Shutdown()
	{
		// Ensure that the submission thread is shutdown before the module is,
		// to allow the thread to drain, so all resources will be destroyed correctly.
		m_submissionThread.Shutdown();
	}
}

Volt::RHI::RHIModule* CreateRHIModule()
{
	return new Volt::RHI::VulkanRHIModule();
}

void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule)
{
	delete reinterpret_cast<Volt::RHI::VulkanRHIModule*>(rhiModule);
}
