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
	}
	
	IntRef<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<StorageBuffer> buffer) const
	{
		IntRef<BufferView> bufferView = IntRef<D3D12BufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	IntRef<BufferView> D3D12RHIModule::CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const
	{
		IntRef<BufferView> bufferView = IntRef<D3D12BufferView>::AttachNoRef(m_bufferViewArena.Allocate(specification, buffer));
		bufferView->SetArena(&m_bufferViewArena);

		return bufferView;
	}

	IntRef<CommandBuffer> D3D12RHIModule::CreateCommandBuffer(QueueType queueType) const
	{
		return IntRef<D3D12CommandBuffer>::Create(queueType);
	}
	
	IntRef<StorageBuffer> D3D12RHIModule::CreateStorageBuffer(const BufferDesc& desc, IntRef<GPUAllocator> allocator) const
	{
		IntRef<D3D12StorageBuffer> buffer = IntRef<D3D12StorageBuffer>::AttachNoRef(m_storageBufferArena.Allocate(desc, allocator));
		buffer->SetArena(&m_storageBufferArena);

		return buffer;
	}

	IntRef<UniformBuffer> D3D12RHIModule::CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData) const
	{
		IntRef<UniformBuffer> uniformBuffer = IntRef<D3D12UniformBuffer>::AttachNoRef(m_uniformBufferArena.Allocate(uniformBufferDesc, initialData));
		uniformBuffer->SetArena(&m_uniformBufferArena);
	
		return uniformBuffer;
	}

	IntRef<GraphicsContext> D3D12RHIModule::CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const
	{
		return IntRef<D3D12GraphicsContext>::Create(createInfo);
	}
	
	IntRef<GraphicsDevice> D3D12RHIModule::CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const
	{
		return IntRef<D3D12GraphicsDevice>::Create(createInfo, physicalGraphicsDevice, enableDebugLayer);
	}
	
	IntRef<PhysicalGraphicsDevice> D3D12RHIModule::CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const
	{
		return IntRef<D3D12PhysicalGraphicsDevice>::Create(createInfo, enableDebugLayer);
	}
	
	IntRef<Swapchain> D3D12RHIModule::CreateSwapchain(const SwapchainCreateInfo& createInfo) const
	{
		return IntRef<D3D12Swapchain>::Create(createInfo);
	}
	
	IntRef<Image> D3D12RHIModule::CreateImage(const ImageDesc& specification, const void* data, IntRef<GPUAllocator> allocator) const
	{
		IntRef<D3D12Image> image = IntRef<D3D12Image>::AttachNoRef(m_imageArena.Allocate(specification, data, allocator));
		image->SetArena(&m_imageArena);

		return image;
	}
	
	IntRef<Image> D3D12RHIModule::CreateImage(const SwapchainImageDesc& specification) const
	{
		IntRef<D3D12Image> image = IntRef<D3D12Image>::AttachNoRef(m_imageArena.Allocate(specification));
		image->SetArena(&m_imageArena);

		return image;
	}

	IntRef<ImageView> D3D12RHIModule::CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const
	{
		IntRef<ImageView> imageView = IntRef<D3D12ImageView>::AttachNoRef(m_imageViewArena.Allocate(specification, image));
		imageView->SetArena(&m_imageViewArena);

		return imageView;
	}

	IntRef<SamplerState> D3D12RHIModule::CreateSamplerState(const SamplerStateDesc& createInfo) const
	{
		IntRef<SamplerState> samplerState = IntRef<D3D12SamplerState>::AttachNoRef(m_samplerStateArena.Allocate(createInfo));
		samplerState->SetArena(&m_samplerStateArena);

		return samplerState;
	}
	
	IntRef<DefaultGPUAllocator> D3D12RHIModule::CreateDefaultAllocator() const
	{
		return IntRef<D3D12DefaultGPUAllocator>::Create();
	}
	
	IntRef<TransientGPUAllocator> D3D12RHIModule::CreateTransientAllocator() const
	{
		return IntRef<D3D12TransientGPUAllocator>::Create();
	}
	
	IntRef<TransientHeap> D3D12RHIModule::CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const
	{
		return IntRef<D3D12TransientHeap>::Create(createInfo);
	}
	
	IntRef<RenderPipeline> D3D12RHIModule::CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const
	{
		return IntRef<D3D12RenderPipeline>::Create(createInfo);
	}
	
	IntRef<ComputePipeline> D3D12RHIModule::CreateComputePipeline(IntRef<Shader> shader) const
	{
		return IntRef<D3D12ComputePipeline>::Create(shader);
	}

	IntRef<RayTracingPipeline> D3D12RHIModule::CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const
	{
		VT_ENSURE(false);

		return IntRef<RayTracingPipeline>();
	}
	
	IntRef<Shader> D3D12RHIModule::CreateShader(const ShaderCreateInfo& specification) const
	{
		return IntRef<D3D12Shader>::Create(specification);
	}
	
	IntRef<ShaderCompiler> D3D12RHIModule::CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const
	{
		return IntRef<D3D12ShaderCompiler>::Create(createInfo);
	}
	
	IntRef<Fence> D3D12RHIModule::CreateFence() const
	{
		return IntRef<D3D12Fence>::Create();
	}

	IntRef<AccelerationStructure> D3D12RHIModule::CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const
	{
		VT_ENSURE(false);

		return IntRef<AccelerationStructure>();
	}

	IntRef<ShaderBindingTable> D3D12RHIModule::CreateShaderBindingTable(IntRef<RayTracingPipeline> pipeline) const
	{
		VT_ENSURE(false);

		return IntRef<ShaderBindingTable>();
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

	IntRef<RayTracingResourceTable> D3D12RHIModule::CreateRayTracingResourceTable() const
	{
		return nullptr;
	}

	void D3D12RHIModule::EndFrame()
	{

	}

	IntRef<CommandBuffer> D3D12RHIModule::CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const
	{
		return nullptr;
	}

	IntRef<Shader> D3D12RHIModule::CreateShaderWithSource(const ShaderCreateInfo& specification, const std::string& source) const
	{
		VT_ENSURE_NO_ENTRY();
		return nullptr;
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
