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
		RefPtr<GPUAllocator> GetDefaultAllocatorImpl() override;
		RefPtr<GPUAllocator> GetTransientAllocatorImpl() override;
		RefPtr<ResourceStateTracker> GetResourceStateTrackerImpl() override;

		RefPtr<GraphicsDevice> GetGraphicsDevice() const override;
		RefPtr<PhysicalGraphicsDevice> GetPhysicalGraphicsDevice() const override;

		void* GetHandleImpl() const override;

	private:
		void Initialize();
		void Shutdown();

		RefPtr<GraphicsDevice> m_graphicsDevice;
		RefPtr<PhysicalGraphicsDevice> m_physicalDevice;
		RefPtr<ResourceStateTracker> m_resourceStateTracker;

		RefPtr<GPUAllocator> m_defaultAllocator;
		RefPtr<GPUAllocator> m_transientAllocator;

		Ref<D3D12DebugLayer> m_debugLayer;

		GraphicsContextCreateInfo m_createInfo{};
	};
}
