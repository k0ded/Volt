#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Shader/Shader.h"

#include <CoreUtilities/Pointers/RefCounted.h>
#include <CoreUtilities/Pointers/RawPtr.h>

#include <span>
#include <CoreUtilities/Weak.h>
#include <CoreUtilities/Core.h>

struct GLFWwindow;

namespace Volt::RHI
{
	class StorageBuffer;
	class UniformBuffer;
	class BufferView;

	class CommandBuffer;

	class DescriptorTable;
	class BindlessDescriptorTable;
	class DeviceQueue;
	class GraphicsContext;
	class GraphicsDevice;
	class PhysicalGraphicsDevice;
	class Swapchain;

	class RayTracingSceneGeometry;
	class AccelerationStructure;
	class ShaderBindingTable;

	class GPUAllocator;
	class DefaultGPUAllocator;
	class TransientGPUAllocator;
	class TransientHeap;

	class Image;
	class ImageView;
	class SamplerState;

	class RenderPipeline;
	class ComputePipeline;
	class RayTracingPipeline;

	class Shader;
	class ShaderCompiler;

	class Event;
	class Fence;
	class Semaphore;

	class ImGuiImplementation;
	class ResourceStateTracker;

	class FrameCapture;

	struct BufferViewDesc;
	struct DescriptorTableCreateInfo;
	struct DeviceQueueCreateInfo;
	struct GraphicsContextCreateInfo;
	struct GraphicsDeviceCreateInfo;
	struct PhysicalDeviceCreateInfo;
	struct ImageDesc;
	struct SwapchainImageDesc;
	struct ImageViewDesc;
	struct SamplerStateDesc;
	struct TransientHeapCreateInfo;
	struct RenderPipelineCreateInfo;
	struct RayTracingPipelineCreateInfo;
	struct ShaderSpecification;
	struct ShaderCompilerCreateInfo;
	struct EventCreateInfo;
	struct FenceCreateInfo;
	struct SemaphoreCreateInfo;
	struct ImGuiCreateInfo;
	struct RayTracingSceneGeometryCreateInfo;
	struct AccelerationStructureCreateInfo;
	struct SwapchainCreateInfo;
	struct ShaderCreateInfo;
	struct BufferDesc;

	struct RHICallbackInfo
	{
		std::function<void()> requestCloseEventCallback;
	};

	class VTRHI_API RHIModule
	{
	public:
		virtual ~RHIModule();

		virtual RefPtr<BufferView> CreateBufferView(const BufferViewDesc& specification) const = 0;

		virtual RefPtr<CommandBuffer> CreateCommandBuffer(QueueType queueType) const = 0;

		virtual RefPtr<StorageBuffer> CreateStorageBuffer(const BufferDesc& desc, RefPtr<GPUAllocator> allocator) const = 0;
		virtual RefPtr<UniformBuffer> CreateUniformBuffer(const uint32_t size, const void* data, const uint32_t count, const std::string& name) const = 0;

		virtual RefPtr<DescriptorTable> CreateDescriptorTable(const DescriptorTableCreateInfo& createInfo) const = 0;
		virtual RefPtr<BindlessDescriptorTable> CreateBindlessDescriptorTable(const uint64_t framesInFlight) const = 0;

		virtual RefPtr<DeviceQueue> CreateDeviceQueue(const DeviceQueueCreateInfo& createInfo) const = 0;
		virtual RefPtr<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const = 0;
		virtual RefPtr<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo) const = 0;
		virtual RefPtr<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo) const = 0;
		virtual RefPtr<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const = 0;

		virtual RefPtr<Image> CreateImage(const ImageDesc& specification, const void* data, RefPtr<GPUAllocator> allocator) const = 0;
		virtual RefPtr<Image> CreateImage(const SwapchainImageDesc& specification) const = 0;

		virtual RefPtr<ImageView> CreateImageView(const ImageViewDesc& specification) const = 0;
		virtual RefPtr<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const = 0;

		virtual RefPtr<DefaultGPUAllocator> CreateDefaultAllocator() const = 0; 
		virtual RefPtr<TransientGPUAllocator> CreateTransientAllocator() const = 0;
		virtual RefPtr<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const = 0;

		virtual RefPtr<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const = 0;
		virtual RefPtr<ComputePipeline> CreateComputePipeline(RefPtr<Shader> shader) const = 0;
		virtual RefPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const = 0;

		virtual RefPtr<Shader> CreateShader(const ShaderCreateInfo& specification) const = 0;
		virtual RefPtr<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const = 0;

		virtual RefPtr<Event> CreateEvent(const EventCreateInfo& createInfo) const = 0;
		virtual RefPtr<Fence> CreateFence(const FenceCreateInfo& createInfo) const = 0;
		virtual RefPtr<Semaphore> CreateSemaphore(const SemaphoreCreateInfo& createInfo) const = 0;

		virtual RefPtr<ImGuiImplementation> CreateImGuiImplementation(const ImGuiCreateInfo& createInfo) const = 0;

		virtual RefPtr<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const = 0;
		virtual RefPtr<ShaderBindingTable> CreateShaderBindingTable(RefPtr<RayTracingPipeline> pipeline) const = 0;

		virtual void SetRHICallbackInfo(const RHICallbackInfo& callbackInfo) = 0;

		virtual void DestroyResource(std::function<void()>&& function) = 0;
		virtual void RequestApplicationClose() = 0;
		virtual void Update() = 0;
		virtual void FlushResourceDeletionQueue() = 0;

		void SetFrameCapture(Ref<FrameCapture> frameCapture);

		static RHIModule& GetInstance() { return *s_instance; }
		static Weak<FrameCapture> GetFrameCapture();

	protected:
		inline static RHIModule* s_instance = nullptr;

		RHIModule();

		Ref<FrameCapture> m_frameCapture;
	};
}
