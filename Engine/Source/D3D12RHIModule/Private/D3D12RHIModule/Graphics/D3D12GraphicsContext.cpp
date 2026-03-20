#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12GraphicsContext.h"
#include "D3D12RHIModule/Graphics/D3D12DebugLayer.h"
#include "D3D12RHIModule/Buffers/CommandSignatureCache.h"
#include "D3D12RHIModule/Descriptors/D3D12DescriptorManager.h"

namespace Volt::RHI
{
	D3D12GraphicsContext::D3D12GraphicsContext(const GraphicsContextCreateInfo& createInfo)
		: m_createInfo(createInfo)
	{
		Initialize();
	}

	D3D12GraphicsContext::~D3D12GraphicsContext()
	{
		Shutdown();
	}

	IntRef<GPUAllocator> D3D12GraphicsContext::GetDefaultAllocatorImpl()
	{
		return m_defaultAllocator;
	}

	IntRef<GPUAllocator> D3D12GraphicsContext::GetTransientAllocatorImpl()
	{
		return m_transientAllocator;
	}

	IntRef<ResourceStateTracker> D3D12GraphicsContext::GetResourceStateTrackerImpl()
	{
		return m_resourceStateTracker;
	}

	IntRef<GraphicsDevice> D3D12GraphicsContext::GetGraphicsDevice() const
	{
		return m_graphicsDevice;
	}

	IntRef<PhysicalGraphicsDevice> D3D12GraphicsContext::GetPhysicalGraphicsDevice() const
	{
		return m_physicalDevice;
	}

	void* D3D12GraphicsContext::GetHandleImpl() const
	{
		return nullptr;
	}

	void D3D12GraphicsContext::Initialize()
	{
		PhysicalDeviceCreateInfo physicalDeviceCreateInfo{};
		m_physicalDevice = PhysicalGraphicsDevice::Create(physicalDeviceCreateInfo, m_createInfo.enableDebugLayer);

#ifdef VT_ENABLE_VALIDATION
		if (m_createInfo.enableDebugLayer)
		{
			m_debugLayer = CreateRef<D3D12DebugLayer>();

			if (!m_debugLayer->IsSupported())
			{
				VT_LOGC(Warning, LogD3D12RHI, "D3D12 debug layer were requested but not supported. Running without it!");
			}
		}
#endif

		GraphicsDeviceCreateInfo graphicsDeviceInfo{};
		m_graphicsDevice = GraphicsDevice::Create(graphicsDeviceInfo, m_physicalDevice, m_createInfo.enableDebugLayer);

#ifdef VT_ENABLE_VALIDATION
		if (m_debugLayer)
		{
			m_debugLayer->InitializeAPIValidation(m_graphicsDevice);
		}
#endif

		m_resourceStateTracker = IntRef<ResourceStateTracker>::Create();
		m_defaultAllocator = DefaultGPUAllocator::Create();
		m_transientAllocator = TransientGPUAllocator::Create();

		g_commandSignatureCache.Initialize();
		g_descriptorManager.Initialize();
	}

	void D3D12GraphicsContext::Shutdown()
	{
		g_descriptorManager.Shutdown();
		g_commandSignatureCache.Shutdown();

		m_transientAllocator = nullptr;
		m_defaultAllocator = nullptr;

		m_debugLayer = nullptr;
	}
}
