#pragma once

#include "RHIModule/Core/RHICommon.h"
#include "RHIModule/Shader/Shader.h"
#include "RHIModule/RHISubmissionThread.h"

#include <CoreUtilities/Pointers/IntRefCounted.h>
#include <CoreUtilities/Pointers/RawPtr.h>

#include <span>
#include <CoreUtilities/Pointers/Weak.h>
#include <CoreUtilities/Core.h>

struct GLFWwindow;

namespace Volt::RHI
{
	class Buffer;
	class TransientBuffer;
	class UniformBuffer;
	class BufferView;

	class CommandBuffer;

	class DeviceQueue;
	class GraphicsContext;
	class GraphicsDevice;
	class PhysicalGraphicsDevice;
	class Swapchain;

	class RayTracingSceneGeometry;
	class AccelerationStructure;
	class ShaderBindingTable;
	class ResourceTable;

	class GPUAllocator;
	class DefaultGPUAllocator;
	class TransientGPUAllocator;
	class TransientHeap;

	class Image;
	class TransientImage;
	class ImageView;
	class SamplerState;

	class RenderPipeline;
	class ComputePipeline;
	class RayTracingPipeline;

	class Shader;
	class ShaderCompiler;

	class Event;
	class Fence;

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
	struct ImGuiCreateInfo;
	struct RayTracingSceneGeometryCreateInfo;
	struct AccelerationStructureCreateInfo;
	struct SwapchainCreateInfo;
	struct ShaderCreateInfo;
	struct BufferDesc;
	struct UniformBufferDesc;
	struct RenderingAttachmentDeclaration;

	struct RHICallbackInfo
	{
		std::function<void()> requestCloseEventCallback;
	};

	class VTRHI_API RHIModule
	{
	public:
		virtual ~RHIModule();

		virtual IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<Buffer> buffer) const = 0;
		virtual IntRef<BufferView> CreateBufferView(const BufferViewDesc& specification, RawPtr<UniformBuffer> buffer) const = 0;

		virtual IntRef<CommandBuffer> CreateCommandBuffer(QueueType queueType) const = 0;
		virtual IntRef<CommandBuffer> CreateSecondaryCommandBuffer(const RenderingAttachmentDeclaration* renderingAttachmentDeclaration) const = 0;

		virtual IntRef<Buffer> CreateBuffer(const BufferDesc& desc) const = 0;
		virtual IntRef<TransientBuffer> CreateTransientBuffer(const BufferDesc& desc) const = 0;
		virtual IntRef<UniformBuffer> CreateUniformBuffer(const UniformBufferDesc& uniformBufferDesc, const void* initialData = nullptr) const = 0;

		virtual IntRef<GraphicsContext> CreateGraphicsContext(const GraphicsContextCreateInfo& createInfo) const = 0;
		virtual IntRef<GraphicsDevice> CreateGraphicsDevice(const GraphicsDeviceCreateInfo& createInfo, RawPtr<PhysicalGraphicsDevice> physicalGraphicsDevice, bool enableDebugLayer) const = 0;
		virtual IntRef<PhysicalGraphicsDevice> CreatePhysicalGraphicsDevice(const PhysicalDeviceCreateInfo& createInfo, bool enableDebugLayer) const = 0;
		virtual IntRef<Swapchain> CreateSwapchain(const SwapchainCreateInfo& createInfo) const = 0;

		virtual IntRef<Image> CreateImage(const ImageDesc& specification, const void* data) const = 0;
		virtual IntRef<Image> CreateImage(const SwapchainImageDesc& specification) const = 0;
		virtual IntRef<TransientImage> CreateTransientImage(const ImageDesc& desc) const = 0;

		virtual IntRef<ImageView> CreateImageView(const ImageViewDesc& specification, RawPtr<Image> image) const = 0;
		virtual IntRef<SamplerState> CreateSamplerState(const SamplerStateDesc& createInfo) const = 0;

		virtual IntRef<DefaultGPUAllocator> CreateDefaultAllocator() const = 0; 
		virtual IntRef<TransientHeap> CreateTransientHeap(const TransientHeapCreateInfo& createInfo) const = 0;

		virtual IntRef<RenderPipeline> CreateRenderPipeline(const RenderPipelineCreateInfo& createInfo) const = 0;
		virtual IntRef<ComputePipeline> CreateComputePipeline(IntRef<Shader> shader) const = 0;
		virtual IntRef<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineCreateInfo& createInfo) const = 0;

		virtual IntRef<Shader> CreateShader(const ShaderCreateInfo& specification) const = 0;
		virtual IntRef<Shader> CreateShaderWithSource(const ShaderCreateInfo& specification, const String& source) const = 0;
		virtual IntRef<ShaderCompiler> CreateShaderCompiler(const ShaderCompilerCreateInfo& createInfo) const = 0;

		virtual IntRef<Fence> CreateFence() const = 0;

		virtual IntRef<ResourceTable> CreateResourceTable() const = 0;

		virtual IntRef<AccelerationStructure> CreateAccelerationStructure(const AccelerationStructureCreateInfo& createInfo) const = 0;
		virtual IntRef<ShaderBindingTable> CreateShaderBindingTable(IntRef<RayTracingPipeline> pipeline) const = 0;

		virtual void SetRHICallbackInfo(const RHICallbackInfo& callbackInfo) = 0;

		virtual void DestroyResource(std::function<void()>&& function) = 0;
		virtual void RequestApplicationClose() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void FlushResourceDeletionQueue() = 0;

		void SetFrameCapture(Ref<FrameCapture> frameCapture);

		static RHIModule& GetInstance() { return *s_instance; }
		static RHISubmissionThread& GetSubmissionThread() { return s_instance->GetSubmissionThreadInternal(); }
		static Weak<FrameCapture> GetFrameCapture();

	protected:
		inline static RHIModule* s_instance = nullptr;

		RHIModule();
		virtual RHISubmissionThread& GetSubmissionThreadInternal() = 0;

		Ref<FrameCapture> m_frameCapture;
	};
}
