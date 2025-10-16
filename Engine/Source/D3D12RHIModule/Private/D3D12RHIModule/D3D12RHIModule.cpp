#include "dxpch.h"
#include "D3D12RHIModule.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsContext.h"
#include "D3D12RHIModule/Graphics/D3D12PhysicalGraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12Swapchain.h"

#include "D3D12RHIModule/Synchronization/D3D12Fence.h"

#include "D3D12RHIModule/Buffers/D3D12CommandBuffer.h"

#include "D3D12RHIModule/Memory/D3D12DefaultGPUAllocator.h"
#include "D3D12RHIModule/Memory/D3D12TransientGPUAllocator.h"
#include "D3D12RHIModule/Memory/D3D12TransientHeap.h"

#include "D3D12RHIModule/Shader/D3D12Shader.h"
#include "D3D12RHIModule/Shader/D3D12ShaderCompiler.h"

#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"

#include "D3D12RHIModule/Pipelines/D3D12RenderPipeline.h"
#include "D3D12RHIModule/Pipelines/D3D12ComputePipeline.h"

#include <RHIModule/RayTracing/AccelerationStructure.h>
#include <RHIModule/RayTracing/RayTracingResuorceTable.h>
#include <RHIModule/Images/SamplerState.h>
#include <RHIModule/Buffers/UniformBuffer.h>
#include <RHIModule/RHICapabilities.h>

namespace Volt::RHI
{
	D3D12RHIModule::D3D12RHIModule()
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
	}
	
	RefPtr<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const
	{
		RefPtr<BufferView> bufferView = RefPtr<D3D12BufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	RefPtr<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const
	{
		RefPtr<BufferView> bufferView = RefPtr<D3D12BufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	RefPtr<CommandBuffer> D3D12RHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		return RefPtr<D3D12CommandBuffer>::Create(queueType);
	}
	
	RefPtr<StorageBuffer> D3D12RHIModule::CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const
	{
		RefPtr<D3D12StorageBuffer> buffer = RefPtr<D3D12StorageBuffer>::AttachNoRef(m_storageBufferArena.Allocate(desc, allocator));
		buffer->SetArena(&m_storageBufferArena);

		return buffer;
	}

	RefPtr<UniformBuffer> D3D12RHIModule::CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const
	{
		RefPtr<UniformBuffer> uniformBuffer = RefPtr<D3D12UniformBuffer>::AttachNoRef(m_uniformBufferArena.Allocate(uniformBufferDesc, initialData));
		uniformBuffer->SetArena(&m_uniformBufferArena);
	
		return uniformBuffer;
	}

	RefPtr<GraphicsContext> D3D12RHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return RefPtr<D3D12GraphicsContext>::Create(createInfo);
	}
	
	RefPtr<GraphicsDevice> D3D12RHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const
	{
		return RefPtr<D3D12GraphicsDevice>::Create(createInfo, physicalGraphicsDevice, enableDebugLayer);
	}
	
	RefPtr<PhysicalGraphicsDevice> D3D12RHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const
	{
		return RefPtr<D3D12PhysicalGraphicsDevice>::Create(createInfo, enableDebugLayer);
	}
	
	RefPtr<Swapchain> D3D12RHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return RefPtr<D3D12Swapchain>::Create(createInfo);
	}
	
	RefPtr<Image> D3D12RHIModule::CreateImage(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator) const
	{
		RefPtr<D3D12Image> image = RefPtr<D3D12Image>::AttachNoRef(m_imageArena.Allocate(specification, data, allocator));
		image->SetArena(&m_imageArena);

		return image;
	}
	
	RefPtr<Image> D3D12RHIModule::CreateImage(const SwapchainImageDesc& specification) const
	{
		RefPtr<D3D12Image> image = RefPtr<D3D12Image>::AttachNoRef(m_imageArena.Allocate(specification));
		image->SetArena(&m_imageArena);

		return image;
	}

	RefPtr<ImageView> D3D12RHIModule::CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const
	{
		RefPtr<ImageView> imageView = RefPtr<D3D12ImageView>::AttachNoRef(m_imageViewArena.Allocate(specification, image));
		imageView->SetArena(&m_imageViewArena);

		return imageView;
	}

	RefPtr<SamplerState> D3D12RHIModule::CreateSamplerState(const SamplerStateDesc& createInfo) const
	{
		RefPtr<SamplerState> samplerState = RefPtr<D3D12SamplerState>::AttachNoRef(m_samplerStateArena.Allocate(createInfo));
		samplerState->SetArena(&m_samplerStateArena);

		return samplerState;
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
		VT_ENSURE(false);

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
		return RefPtr<D3D12Fence>::Create();
	}

	RefPtr<AccelerationStructure> D3D12RHIModule::CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const
	{
		VT_ENSURE(false);

		return RefPtr<AccelerationStructure>();
	}

	RefPtr<ShaderBindingTable> D3D12RHIModule::CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const
	{
		VT_ENSURE(false);

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

		g_descriptorManager.BeginFrame();

		const uint32_t queueIndex = m_frameIndex % RHI::RHICapabilities::NumFramesInFlight;
		m_resourceDeletionQueue.FlushQueue(queueIndex);

		m_frameIndex++;
	}

	void D3D12RHIModule::FlushResourceDeletionQueue()
	{
		m_resourceDeletionQueue.FlushAll();
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
