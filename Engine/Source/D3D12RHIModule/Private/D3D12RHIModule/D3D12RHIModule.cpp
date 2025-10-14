#include "dxpch.h"
#include "D3D12RHIModule.h"

#include "D3D12RHIModule/Graphics/D3D12Swapchain.h"
#include "D3D12RHIModule/Graphics/D3D12DeviceQueue.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsContext.h"
#include "D3D12RHIModule/Graphics/D3D12PhysicalGraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include "D3D12RHIModule/Buffers/D3D12UniformBuffer.h"
#include "D3D12RHIModule/Buffers/D3D12StorageBuffer.h"
#include "D3D12RHIModule/Buffers/D3D12BufferView.h"
#include "D3D12RHIModule/Buffers/D3D12CommandBuffer.h"

#include "D3D12RHIModule/ImGui/D3D12ImGuiImplementation.h"

#include "D3D12RHIModule/Shader/D3D12ShaderCompiler.h"
#include "D3D12RHIModule/Shader/D3D12Shader.h"

#include "D3D12RHIModule/Pipelines/D3D12RenderPipeline.h"
#include "D3D12RHIModule/Pipelines/D3D12ComputePipeline.h"

#include "D3D12RHIModule/Memory/D3D12TransientHeap.h"
#include "D3D12RHIModule/Memory/D3D12TransientGPUAllocator.h"
#include "D3D12RHIModule/Memory/D3D12DefaultGPUAllocator.h"

#include "D3D12RHIModule/Images/D3D12SamplerState.h"
#include "D3D12RHIModule/Images/D3D12ImageView.h"
#include "D3D12RHIModule/Images/D3D12Image.h"

#include "D3D12RHIModule/Synchronization/D3D12Semaphore.h"

#include <RHIModule/RayTracing/AccelerationStructure.h>
#include <RHIModule/Synchronization/Fence.h>
#include <RHIModule/RHICapabilities.h>

namespace Volt::RHI
{
	D3D12RHIModule::D3D12RHIModule()
	{
		s_instance = this;
		m_resourceDeletionQueue.SetSize(RHI::RHICapabilities::NumFramesInFlight);
	}
	
	RefPtr<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const
	{
		return RefPtr<D3D12BufferView>::Create(specification, buffer);
	}

	RefPtr<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const
	{
		return RefPtr<D3D12BufferView>::Create(specification, buffer);
	}

	RefPtr<CommandBuffer> D3D12RHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		return RefPtr<D3D12CommandBuffer>::Create(queueType);
	}
	
	RefPtr<StorageBuffer> D3D12RHIModule::CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const
	{
		return RefPtr<D3D12StorageBuffer>::Create(desc, allocator);
	}

	RefPtr<UniformBuffer> D3D12RHIModule::CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const
	{
		return RefPtr<D3D12UniformBuffer>::Create(uniformBufferDesc, initialData);
	}

	RefPtr<DeviceQueue> D3D12RHIModule::CreateDeviceQueue(const DeviceQueueCreateInfo& createInfo) const
	{
		return RefPtr<DeviceQueue>();
	}
	
	RefPtr<GraphicsContext> D3D12RHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return RefPtr<D3D12GraphicsContext>::Create(createInfo);
	}
	
	RefPtr<GraphicsDevice> D3D12RHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo) const
	{
		return RefPtr<D3D12GraphicsDevice>::Create(createInfo);
	}
	
	RefPtr<PhysicalGraphicsDevice> D3D12RHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo) const
	{
		return RefPtr<D3D12PhysicalGraphicsDevice>::Create(createInfo);
	}
	
	RefPtr<Swapchain> D3D12RHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return RefPtr<D3D12Swapchain>::Create(createInfo);
	}
	
	RefPtr<Image> D3D12RHIModule::CreateImage(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator) const
	{
		return RefPtr<D3D12Image>::Create(specification, data, allocator);
	}
	
	RefPtr<Image> D3D12RHIModule::CreateImage(const SwapchainImageDesc& specification) const
	{
		return RefPtr<D3D12Image>::Create(specification);
	}

	RefPtr<ImageView> D3D12RHIModule::CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const
	{
		return RefPtr<D3D12ImageView>::Create(specification, image);
	}

	RefPtr<SamplerState> D3D12RHIModule::CreateSamplerState(const SamplerStateDesc& createInfo) const
	{
		return RefPtr<D3D12SamplerState>::Create(createInfo);
	}
	
	RefPtr<DefaultGPUAllocator> D3D12RHIModule::CreateDefaultAllocator() const
	{
		return RefPtr<D3D12DefaultGPUAllocator>::Create();
	}
	
	RefPtr<TransientGPUAllocator> D3D12RHIModule::CreateTransientAllocator() const
	{
		return RefPtr<D3D12TransientGPUAllocator>::Create();
	}
	
	RefPtr<TransientHeap> D3D12RHIModule::CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const
	{
		return RefPtr<D3D12TransientHeap>::Create(createInfo);
	}
	
	RefPtr<RenderPipeline> D3D12RHIModule::CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const
	{
		return RefPtr<D3D12RenderPipeline>::Create(createInfo);
	}
	
	RefPtr<ComputePipeline> D3D12RHIModule::CreateComputePipeline(RefPtr<Shader> shader) const
	{
		return RefPtr<D3D12ComputePipeline>::Create(shader);
	}

	RefPtr<RayTracingPipeline> D3D12RHIModule::CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
	{
		return RefPtr<RayTracingPipeline>();
	}
	
	RefPtr<Shader> D3D12RHIModule::CreateShader(const ShaderCreateInfo& specification) const
	{
		return RefPtr<D3D12Shader>::Create(specification);
	}
	
	RefPtr<ShaderCompiler> D3D12RHIModule::CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const
	{
		return RefPtr<D3D12ShaderCompiler>::Create(createInfo);
	}
	
	RefPtr<Fence> D3D12RHIModule::CreateFence() const
	{
		return RefPtr<Fence>();
	}

	RefPtr<Semaphore> D3D12RHIModule::CreateSemaphore(const SemaphoreCreateInfo& createInfo) const
	{
		return RefPtr<D3D12Semaphore>::Create(createInfo);
	}

	RefPtr<AccelerationStructure> D3D12RHIModule::CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const
	{
		return RefPtr<AccelerationStructure>();
	}

	RefPtr<ShaderBindingTable> D3D12RHIModule::CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const
	{
		return RefPtr<ShaderBindingTable>();
	}
	
	void D3D12RHIModule::SetRHICallbackInfo(const RHICallbackInfo& callbackInfo)
	{
		m_callbackInfo = callbackInfo;
	}

	void D3D12RHIModule::DestroyResource(std::function<void()>&& function)
	{
		const uint32_t queueIndex = m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		m_resourceDeletionQueue.EnqueueResourceDeletion(queueIndex, std::move(function));
	}

	void D3D12RHIModule::RequestApplicationClose()
	{
		if (m_callbackInfo.requestCloseEventCallback)
		{
			m_callbackInfo.requestCloseEventCallback();
		}
	}

	void D3D12RHIModule::BeginFrame()
	{
		GraphicsContext::GetDefaultAllocator()->Update();
		GraphicsContext::GetTransientAllocator()->Update();

		const uint32_t queueIndex = m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		m_resourceDeletionQueue.FlushQueue(queueIndex);

		m_frameIndex++;
	}

	void D3D12RHIModule::FlushResourceDeletionQueue()
	{
		m_resourceDeletionQueue.FlushAll();
	}

	RefPtr<Shader> D3D12RHIModule::CreateShader(const ShaderCreateInfo& specification) const
	{
		return nullptr;
	}

	RefPtr<RenderPipeline> D3D12RHIModule::CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const
	{
		return nullptr;
	}

	RefPtr<RayTracingResourceTable> D3D12RHIModule::CreateRayTracingResourceTable() const
	{
		return nullptr;
	}

	void D3D12RHIModule::EndFrame()
	{

	}

}

Volt::RHI::RHIModule* CreateRHIModule()
{
	return new Volt::RHI::D3D12RHIModule();
}

void DestroyRHIModule(Volt::RHI::RHIModule* rhiModule)
{
	delete reinterpret_cast<Volt::RHI::D3D12RHIModule*>(rhiModule);
}
