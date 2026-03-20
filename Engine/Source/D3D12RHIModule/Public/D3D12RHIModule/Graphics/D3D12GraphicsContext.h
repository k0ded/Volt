#pragma once

#include "D3D12RHIModule/Core.h"

#include <RHIModule/Graphics/GraphicsContext.h>

namespace Volt::RHI
{
	class PhysicalGraphicsDevice;
	class GraphicsDevice;

	class D3D12DebugLayer;

	class D3D12GraphicsContext final : public GraphicsContext
	{
	public:
		D3D12GraphicsContext(const GraphicsContextCreateInfo& createInfo);
		~D3D12GraphicsContext() override;

	protected:
		IntRef<GPUAllocator> GetDefaultAllocatorImpl() override;
		IntRef<GPUAllocator> GetTransientAllocatorImpl() override;
		IntRef<ResourceStateTracker> GetResourceStateTrackerImpl() override;

		IntRef<GraphicsDevice> GetGraphicsDevice() const override;
		IntRef<PhysicalGraphicsDevice> GetPhysicalGraphicsDevice() const override;

		void* GetHandleImpl() const override;

	private:
		void Initialize();
		void Shutdown();

		IntRef<GraphicsDevice> m_graphicsDevice;
		IntRef<PhysicalGraphicsDevice> m_physicalDevice;
		IntRef<ResourceStateTracker> m_resourceStateTracker;

		IntRef<GPUAllocator> m_defaultAllocator;
		IntRef<GPUAllocator> m_transientAllocator;

		Ref<D3D12DebugLayer> m_debugLayer;

		GraphicsContextCreateInfo m_createInfo{};
	};
}
