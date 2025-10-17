#include "vkpch.h"
#include "VulkanRHIModule.h"

#include "VulkanRHIModule/Buffers/VulkanBufferView.h"
#include "VulkanRHIModule/Buffers/VulkanCommandBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanUniformBuffer.h"
#include "VulkanRHIModule/Buffers/VulkanStorageBuffer.h"

#include "VulkanRHIModule/Descriptors/VulkanDescriptorHeap.h"

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

#include "VulkanRHIModule/Synchronization/VulkanFence.h"

#include "VulkanRHIModule/RayTracing/VulkanAccelerationStructure.h"
#include "VulkanRHIModule/RayTracing/VulkanShaderBindingTable.h"
#include "VulkanRHIModule/RayTracing/VulkanRayTracingResourceTable.h"

#include "VulkanRHIModule/Common/VulkanCPUAllocator.h"

namespace Volt::RHI
{
	VulkanRHIModule::VulkanRHIModule()
	{
		s_instance = this;
		m_resourceDeletionQueue.SetSize(RHI::RHICapabilities::NumFramesInFlight);

		// Allocate arenas
		constexpr size_t ArenaSize = 8192;

		m_bufferViewArena.AllocateArena(ArenaSize);
		m_imageViewArena.AllocateArena(ArenaSize);

		m_storageBufferArena.AllocateArena(ArenaSize);
		m_uniformBufferArena.AllocateArena(ArenaSize);
		m_imageArena.AllocateArena(ArenaSize);
		m_samplerStateArena.AllocateArena(ArenaSize);

		m_vulkanCpuAllocator = CreateRef<VulkanCPUAllocator>();
	}

	RefPtr<BufferView> VulkanRHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const
	{
		RefPtr<BufferView> bufferView = RefPtr<VulkanBufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	RefPtr<BufferView> VulkanRHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const
	{
		RefPtr<BufferView> bufferView = RefPtr<VulkanBufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	RefPtr<CommandBuffer> VulkanRHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		return RefPtr<VulkanCommandBuffer>::Create(queueType);
	}

	RefPtr<StorageBuffer> VulkanRHIModule::CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const
	{
		RefPtr<StorageBuffer> storageBuffer = RefPtr<VulkanStorageBuffer>::AttachNoRef(m_storageBufferArena.Allocate(desc, allocator));
		storageBuffer->SetArena(&m_storageBufferArena);
		return storageBuffer;
	}

	RefPtr<UniformBuffer> VulkanRHIModule::CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData = nullptr) const
	{
		RefPtr<UniformBuffer> uniformBuffer = RefPtr<VulkanUniformBuffer>::AttachNoRef(m_uniformBufferArena.Allocate(uniformBufferDesc, initialData));
		uniformBuffer->SetArena(&m_uniformBufferArena);
		return uniformBuffer;
	}

	RefPtr<GraphicsContext> VulkanRHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return RefPtr<VulkanGraphicsContext>::Create(createInfo);
	}

	RefPtr<GraphicsDevice> VulkanRHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const
	{
		return RefPtr<VulkanGraphicsDevice>::Create(createInfo, physicalGraphicsDevice, enableDebugLayer);
	}

	RefPtr<PhysicalGraphicsDevice> VulkanRHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const
	{
		return RefPtr<VulkanPhysicalGraphicsDevice>::Create(createInfo, enableDebugLayer);
	}

	RefPtr<Swapchain> VulkanRHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return RefPtr<VulkanSwapchain>::Create(createInfo);
	}

	RefPtr<Image> VulkanRHIModule::CreateImage(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator) const
	{
		RefPtr<VulkanImage> image = RefPtr<VulkanImage>::AttachNoRef(m_imageArena.Allocate(specification, data, allocator));
		image->SetArena(&m_imageArena);

		return image;
	}

	RefPtr<Image> VulkanRHIModule::CreateImage(const SwapchainImageDesc& specification) const
	{
		RefPtr<VulkanImage> image = RefPtr<VulkanImage>::AttachNoRef(m_imageArena.Allocate(specification));
		image->SetArena(&m_imageArena);

		return image;
	}

	RefPtr<ImageView> VulkanRHIModule::CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const
	{
		RefPtr<ImageView> imageView = RefPtr<VulkanImageView>::AttachNoRef(m_imageViewArena.Allocate(specification, image));
		imageView->SetArena(&m_imageViewArena);

		return imageView;
	}

	RefPtr<SamplerState> VulkanRHIModule::CreateSamplerState(const SamplerStateDesc& createInfo) const
	{
		RefPtr<SamplerState> samplerState = RefPtr<VulkanSamplerState>::AttachNoRef(m_samplerStateArena.Allocate(createInfo));
		samplerState->SetArena(&m_samplerStateArena);

		return samplerState;
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

	RefPtr<Fence> VulkanRHIModule::CreateFence() const
	{
		return RefPtr<VulkanFence>::Create();
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
		const uint32_t queueIndex = m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		m_resourceDeletionQueue.EnqueueResourceDeletion(queueIndex, std::move(function));
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
		GraphicsContext::GetDefaultAllocator()->Update();
		GraphicsContext::GetTransientAllocator()->Update();

		VulkanGraphicsContext& vkGraphicsContext = GraphicsContext::Get().AsRef<VulkanGraphicsContext>();
		vkGraphicsContext.GetDescriptorHeap().BeginFrame();

		const uint32_t queueIndex = ++m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		m_resourceDeletionQueue.FlushQueue(queueIndex);
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

	RefPtr<RayTracingResourceTable> VulkanRHIModule::CreateRayTracingResourceTable() const
	{
		return RefPtr<VulkanRayTracingResourceTable>::Create();
	}

	void VulkanRHIModule::EndFrame()
	{
		VulkanGraphicsContext& vkGraphicsContext = GraphicsContext::Get().AsRef<VulkanGraphicsContext>();
		vkGraphicsContext.GetDescriptorHeap().Flush();
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
