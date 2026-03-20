#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12Swapchain.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"
#include "D3D12RHIModule/Graphics/D3D12DeviceQueue.h"

#include <RHIModule/Utility/ResourceUtility.h>
#include <RHIModule/RHICapabilities.h>
#include <RHIModule/Synchronization/Fence.h>

#include <CoreUtilities/Profiling/Profiling.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>

// #TODO_D3D12: Implement better back buffer format support

namespace Volt::RHI
{
	namespace Utility
	{
		inline bool GetSupportsTearing()
		{
			bool result = false;

			ComPtr<IDXGIFactory4> factory4;
			if (SUCCEEDED(CreateDXGIFactory1(VT_D3D12_ID(factory4))))
			{
				ComPtr<IDXGIFactory5> factory5;
				if (SUCCEEDED(factory4.As(&factory5)))
				{
					if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &result, sizeof(result))))
					{
						result = false;
					}
				}
			}

			return result;
		}
	}

	D3D12Swapchain::D3D12Swapchain(const SwapchainCreateInfo& createInfo)
		: m_createInfo(createInfo), m_width(createInfo.width), m_height(createInfo.height), m_VSyncEnabled(m_createInfo.enableVSync)
	{
		VT_ENSURE_MSG(!createInfo.useHDRIfAvailable, "HDR is currently not supported!");

		m_windowHandle = glfwGetWin32Window(reinterpret_cast<GLFWwindow*>(createInfo.platformWindow));

		m_commandBuffers.resize(RHI::RHICapabilities::NumFramesInFlight);
		m_renderFences.resize(RHI::RHICapabilities::NumFramesInFlight);
		for (uint32_t i = 0; i < RHI::RHICapabilities::NumFramesInFlight; i++)
		{
			m_commandBuffers[i] = CommandBuffer::Create();
			m_renderFences[i] = Fence::Create();
		}

		ID3D12Device10* d3d12Device = GraphicsContext::GetDevice()->AsRef<D3D12GraphicsDevice>().GetDevice10();
		VT_D3D12_CHECK(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_SHARED, VT_D3D12_ID(m_presentFence)));
		m_windowsPresentFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

		Invalidate(m_width, m_height);
	}

	D3D12Swapchain::~D3D12Swapchain()
	{
		if (m_windowsPresentFenceEvent)
		{
			::CloseHandle(m_windowsPresentFenceEvent);
		}

		Release();
	}

	void* D3D12Swapchain::GetHandleImpl() const
	{
		return m_swapchain.Get();
	}

	void D3D12Swapchain::BeginFrame()
	{
		VT_PROFILE_FUNCTION();
	
		m_currentImageIndex = m_swapchain->GetCurrentBackBufferIndex();

		m_renderFences.at(m_currentFrameIndex)->WaitUntilSignaled();
		m_renderFences.at(m_currentFrameIndex)->Reset();
		m_commandBuffers.at(m_currentFrameIndex)->Begin();
	}

	void D3D12Swapchain::Present()
	{
		VT_PROFILE_FUNCTION();

		{
			ResourceBarrierInfo barrier = ResourceBarrierInfo::InitializeAsImageBarrier();
			ResourceUtility::InitializeBarrierSrcFromCurrentState(barrier.imageBarrier(), m_perImageData.at(m_currentImageIndex).imageReference);
			barrier.imageBarrier().dstAccess = BarrierAccess::AllRead;
			barrier.imageBarrier().dstStage = BarrierStage::AllGraphics;
			barrier.imageBarrier().dstLayout = ImageLayout::Present;
			barrier.imageBarrier().resource = m_perImageData.at(m_currentImageIndex).imageReference;

			m_commandBuffers.at(m_currentFrameIndex)->ResourceBarrier({ barrier });
		}

		m_commandBuffers.at(m_currentFrameIndex)->End();

		DeviceQueueExecuteInfo executeInfo{};
		executeInfo.commandBuffers = { m_commandBuffers.at(m_currentFrameIndex) };
		executeInfo.executionFence = m_renderFences.at(m_currentFrameIndex);

		D3D12DeviceQueue& deviceQueue = GraphicsContext::GetDevice()->GetDeviceQueue(QueueType::Graphics)->AsRef<D3D12DeviceQueue>();
		deviceQueue.Execute(executeInfo);

		VT_D3D12_CHECK(m_swapchain->Present(m_VSyncEnabled ? 1 : 0, m_supportsTearing ? DXGI_PRESENT_ALLOW_TEARING : 0));

		deviceQueue.SignalFence(m_presentFence, ++m_presentFenceValue);

		GetNextFrameIndex();
	}

	void D3D12Swapchain::Resize(const uint32_t width, const uint32_t height, bool enableVSync)
	{
		VT_PROFILE_FUNCTION();

		m_width = width;
		m_height = height;
		m_VSyncEnabled = enableVSync;

		if (m_presentFence->GetCompletedValue() < m_presentFenceValue)
		{
			m_presentFence->SetEventOnCompletion(m_presentFenceValue, m_windowsPresentFenceEvent);
			::WaitForSingleObject(m_windowsPresentFenceEvent, INFINITE);
			::ResetEvent(m_windowsPresentFenceEvent);
		}

		for (IntRef<Fence> fence : m_renderFences)
		{
			fence->WaitUntilSignaled();
		}

		GraphicsContext::GetDevice()->GetDeviceQueue(QueueType::Graphics)->WaitForQueue();

		for (uint32_t i = 0; i < RHI::RHICapabilities::NumFramesInFlight; i++)
		{
			m_perImageData[i].imageReference = nullptr;
			m_perImageData[i].resource.Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		VT_D3D12_CHECK(m_swapchain->GetDesc(&swapChainDesc));
		VT_D3D12_CHECK(m_swapchain->ResizeBuffers(RHI::RHICapabilities::NumFramesInFlight, m_width, m_height, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		GetSwapchainImages();
	}

	const uint32_t D3D12Swapchain::GetCurrentFrame() const
	{
		return m_currentFrameIndex;
	}

	const uint32_t D3D12Swapchain::GetWidth() const
	{
		return m_width;
	}

	const uint32_t D3D12Swapchain::GetHeight() const
	{
		return m_height;
	}

	IntRef<Image> D3D12Swapchain::GetCurrentImage() const
	{
		return m_perImageData[m_currentImageIndex].imageReference;
	}

	const PixelFormat D3D12Swapchain::GetFormat() const
	{
		return PixelFormat::R8G8B8A8_UNORM;
	}

	bool D3D12Swapchain::IsHDREnabled() const
	{
		return false;
	}

	void D3D12Swapchain::Invalidate(const uint32_t width, const uint32_t height)
	{
		CreateSwapchain(width, height);
		GetSwapchainImages();
	}

	void D3D12Swapchain::Release()
	{
		for (IntRef<Fence> fence : m_renderFences)
		{
			fence->WaitUntilSignaled();
		}

		GraphicsContext::GetDevice()->GetDeviceQueue(QueueType::Graphics)->WaitForQueue();

		m_commandBuffers.clear();
		m_perImageData.clear();
		m_swapchain = nullptr;
	}

	void D3D12Swapchain::CreateSwapchain(const uint32_t width, const uint32_t height)
	{
		ComPtr<IDXGIFactory4> factory;
		VT_D3D12_CHECK(CreateDXGIFactory2(0, VT_D3D12_ID(factory)));

		m_supportsTearing = Utility::GetSupportsTearing();

		DXGI_SWAP_CHAIN_DESC1 swapchainDesc{};
		swapchainDesc.Width = width;
		swapchainDesc.Height = height;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.Stereo = false;
		swapchainDesc.SampleDesc = { 1, 0 };
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.BufferCount = RHI::RHICapabilities::NumFramesInFlight;
		swapchainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		swapchainDesc.Flags = m_supportsTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ID3D12CommandQueue* cmdQueue = GraphicsContext::GetDevice()->GetDeviceQueue(QueueType::Graphics)->GetHandle<ID3D12CommandQueue*>();

		ComPtr<IDXGISwapChain1> swapchain1;
		VT_D3D12_CHECK(factory->CreateSwapChainForHwnd(cmdQueue, m_windowHandle, &swapchainDesc, nullptr, nullptr, &swapchain1));
		VT_D3D12_CHECK(factory->MakeWindowAssociation(m_windowHandle, DXGI_MWA_NO_ALT_ENTER));
		VT_D3D12_CHECK(swapchain1.As(&m_swapchain));
	}

	void D3D12Swapchain::GetNextFrameIndex()
	{
		m_currentFrameIndex = (m_currentFrameIndex + 1) % RHI::RHICapabilities::NumFramesInFlight;
	}

	void D3D12Swapchain::GetSwapchainImages()
	{
		m_perImageData.resize(RHI::RHICapabilities::NumFramesInFlight);

		for (uint32_t i = 0; i < RHI::RHICapabilities::NumFramesInFlight; i++)
		{
			ComPtr<ID3D12Resource> backBuffer;
			m_swapchain->GetBuffer(i, VT_D3D12_ID(backBuffer));

			const std::wstring name = L"Swapchain Target - Index " + std::to_wstring(i);
			backBuffer->SetName(name.c_str());

			m_perImageData[i].resource = backBuffer;

			SwapchainImageDesc spec{};
			spec.swapchain = this;
			spec.imageIndex = i;
			m_perImageData[i].imageReference = Image::Create(spec);
		}
	}
}
