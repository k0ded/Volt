#include "vkpch.h"
#include "VulkanRHIModule.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanIndexBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanVertexBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorTable.h"
#include "VulkanRHIModule/Descriptors/VulkanBindlessDescriptorTable.h"

#include "VulkanRHIModule/Graphics/VulkanDeviceQueue.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsContext.h"
#include "VulkanRHIModule/Graphics/VulkanGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanPhysicalGraphicsDevice.h"
#include "VulkanRHIModule/Graphics/VulkanSwapchain.h"

#include "VulkanRHIModule/Images/VulkanImage.h"
#include "VulkanRHIModule/Images/VulkanImageView.h"
#include "VulkanRHIModule/Images/VulkanSamplerState.h"

#include "VulkanRHIModule/Memory/VulkanDefaultGPUAllocator.h"
#include "VulkanRHIModule/Memory/VulkanTransientGPUAllocator.h"
#include "VulkanRHIModule/Memory/VulkanTransientHeap.h"

#include "VulkanRHIModule/Pipelines/VulkanRenderPipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanComputePipeline.h"
#include "VulkanRHIModule/Pipelines/VulkanRayTracingPipeline.h"

#include "VulkanRHIModule/Shader/VulkanShader.h"
#include "VulkanRHIModule/Shader/VulkanShaderCompiler.h"

#include "VulkanRHIModule/Synchronization/VulkanEvent.h"
#include "VulkanRHIModule/Synchronization/VulkanFence.h"
#include "VulkanRHIModule/Synchronization/VulkanSemaphore.h"

#include "VulkanRHIModule/RayTracing/VulkanAccelerationStructure.h"
#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"

#include "VulkanRHIModule/ImGui/VulkanImGuiImplementation.h"

namespace Volt::RHI
{
	VulkanRHIModule::VulkanRHIModule()
	{
		s_instance = this;
		m_resourceDeletionQueue.SetSize(RHI::Swapchain::FramesInFlight);
	}

	RefPtr<BufferView> VulkanRHIModule::CreateBufferView(const BufferViewDesc& specification) const
	{
		return RefPtr<VulkanBufferView>::Create(specification);
	}

	RefPtr<CommandBuffer> VulkanRHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		return RefPtr<VulkanCommandBuffer>::Create(queueType);
	}

	RefPtr<IndexBuffer> VulkanRHIModule::CreateIndexBuffer(std::span<const uint32_t> indices) const
	{
		return RefPtr<VulkanIndexBuffer>::Create(indices);
	}

	RefPtr<VertexBuffer> VulkanRHIModule::CreateVertexBuffer(const void* data, const uint32_t size, const uint32_t stride) const
	{
		return RefPtr<VulkanVertexBuffer>::Create(data, size, stride);
	}

	RefPtr<StorageBuffer> VulkanRHIModule::CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const
	{
		return RefPtr<VulkanStorageBuffer>::Create(desc, allocator);
	}

	RefPtr<UniformBuffer> VulkanRHIModule::CreateUniformBuffer(const uint32_t size, const void* data, const uint32_t count, const std::string& name) const
	{
		return RefPtr<VulkanUniformBuffer>::Create(size, data, count, name);
	}

	RefPtr<BindlessDescriptorTable> VulkanRHIModule::CreateBindlessDescriptorTable(const uint64_t framesInFlight) const
	{
		return RefPtr<VulkanBindlessDescriptorTable>::Create(framesInFlight);
	}

	RefPtr<DeviceQueue> VulkanRHIModule::CreateDeviceQueue(const DeviceQueueCreateInfo& createInfo) const
	{
		return RefPtr<VulkanDeviceQueue>::Create(createInfo);
	}

	RefPtr<GraphicsContext> VulkanRHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return RefPtr<VulkanGraphicsContext>::Create(createInfo);
	}

	RefPtr<GraphicsDevice> VulkanRHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo) const
	{
		return RefPtr<VulkanGraphicsDevice>::Create(createInfo);
	}

	RefPtr<PhysicalGraphicsDevice> VulkanRHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo) const
	{
		return RefPtr<VulkanPhysicalGraphicsDevice>::Create(createInfo);
	}

	RefPtr<Swapchain> VulkanRHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return RefPtr<VulkanSwapchain>::Create(createInfo);
	}

	RefPtr<Image> VulkanRHIModule::CreateImage(const ImageSpecification& specification, const void* data, RefPtr<GPUAllocator> allocator) const
	{
		return RefPtr<VulkanImage>::Create(specification, data, allocator);
	}

	RefPtr<Image> VulkanRHIModule::CreateImage(const SwapchainImageSpecification& specification) const
	{
		return RefPtr<VulkanImage>::Create(specification);
	}

	RefPtr<ImageView> VulkanRHIModule::CreateImageView(const ImageViewDesc& specification) const
	{
		return RefPtr<VulkanImageView>::Create(specification);
	}

	RefPtr<SamplerState> VulkanRHIModule::CreateSamplerState(const SamplerStateCreateInfo& createInfo) const
	{
		return RefPtr<VulkanSamplerState>::Create(createInfo);
	}

	RefPtr<DefaultGPUAllocator> VulkanRHIModule::CreateDefaultAllocator() const
	{
		return RefPtr<VulkanDefaultGPUAllocator>::Create();
	}

	RefPtr<TransientGPUAllocator> VulkanRHIModule::CreateTransientAllocator() const
	{
		return RefPtr<VulkanTransientGPUAllocator>::Create();
	}

	RefPtr<TransientHeap> VulkanRHIModule::CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const
	{
		return RefPtr<VulkanTransientHeap>::Create(createInfo);
	}

	RefPtr<Volt::RHI::ComputePipeline> VulkanRHIModule::CreateComputePipeline(RefPtr<Shader> shader) const
	{
		return RefPtr<VulkanComputePipeline>::Create(shader);
	}

	RefPtr<RayTracingPipeline> VulkanRHIModule::CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
	{
		return RefPtr<VulkanRayTracingPipeline>::Create(createInfo);
	}

	RefPtr<ShaderCompiler> VulkanRHIModule::CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const
	{
		return RefPtr<VulkanShaderCompiler>::Create(createInfo);
	}

	RefPtr<Event> VulkanRHIModule::CreateEvent(const EventCreateInfo& createInfo) const
	{
		return RefPtr<VulkanEvent>::Create(createInfo);
	}

	RefPtr<Fence> VulkanRHIModule::CreateFence(const FenceCreateInfo& createInfo) const
	{
		return RefPtr<VulkanFence>::Create(createInfo);
	}

	RefPtr<Semaphore> VulkanRHIModule::CreateSemaphore(const SemaphoreCreateInfo& createInfo) const
	{
		return RefPtr<VulkanSemaphore>::Create(createInfo);
	}

	RefPtr<ImGuiImplementation> VulkanRHIModule::CreateImGuiImplementation(const ImGuiCreateInfo& createInfo) const
	{
		return RefPtr<VulkanImGuiImplementation>::Create(createInfo);
	}

	RefPtr<AccelerationStructure> VulkanRHIModule::CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const
	{
		return RefPtr<VulkanAccelerationStructure>::Create(createInfo);
	}

	RefPtr<ShaderBindingTable> VulkanRHIModule::CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const
	{
		return RefPtr<VulkanShaderBindingTable>::Create(pipeline);
	}

	void VulkanRHIModule::SetRHICallbackInfo(const RHICallbackInfo& callbackInfo)
	{
		m_callbackInfo = callbackInfo;
	}

	void VulkanRHIModule::DestroyResource(std::function<void()>&& function)
	{
		const uint32_t queueIndex = m_frameIndex % RHI::Swapchain::FramesInFlight;
		m_resourceDeletionQueue.EnqueueResourceDeletion(queueIndex, std::move(function));
	}

	void VulkanRHIModule::RequestApplicationClose()
	{
		if (m_callbackInfo.requestCloseEventCallback)
		{
			m_callbackInfo.requestCloseEventCallback();
		}
	}

	void VulkanRHIModule::Update()
	{
		GraphicsContext::GetDefaultAllocator()->Update();
		GraphicsContext::GetTransientAllocator()->Update();

		const uint32_t queueIndex = m_frameIndex % RHI::Swapchain::FramesInFlight;
		m_resourceDeletionQueue.FlushQueue(queueIndex);

		m_frameIndex++;
	}

	void VulkanRHIModule::FlushResourceDeletionQueue()
	{
		m_resourceDeletionQueue.FlushAll();
	}

	RefPtr<Shader> VulkanRHIModule::CreateShader(const ShaderCreateInfo& specification) const
	{
		return RefPtr<VulkanShader>::Create(specification);
	}

	RefPtr<RenderPipeline> VulkanRHIModule::CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const
	{
		return RefPtr<VulkanRenderPipeline>::Create(createInfo);
	}

	RefPtr<DescriptorTable> VulkanRHIModule::CreateDescriptorTable(const DescriptorTableCreateInfo& createInfo) const
	{
		return RefPtr<VulkanDescriptorTable>::Create(createInfo);
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
