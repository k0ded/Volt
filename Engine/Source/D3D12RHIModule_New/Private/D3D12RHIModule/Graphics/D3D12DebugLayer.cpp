#include "dxpch.h"

#include "D3D12RHIModule/Graphics/D3D12DebugLayer.h"
#include "D3D12RHIModule/Graphics/D3D12GraphicsDevice.h"

#include <d3d12.h>

namespace Volt::RHI
{
	void LogD3D12Message(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR pDescription, void* context)
	{
		switch (severity)
		{
			case D3D12_MESSAGE_SEVERITY_CORRUPTION:
				VT_LOGC(Error, LogD3D12RHI, std::string("D3D12 Validation:") + std::string(pDescription));
				break;
			case D3D12_MESSAGE_SEVERITY_ERROR:
				VT_LOGC(Error, LogD3D12RHI, std::string("D3D12 Validation:") + std::string(pDescription));
				break;
			case D3D12_MESSAGE_SEVERITY_WARNING:
				VT_LOGC(Warning, LogD3D12RHI, std::string("D3D12 Validation:") + std::string(pDescription));
				break;
			case D3D12_MESSAGE_SEVERITY_INFO:
				VT_LOGC(Info, LogD3D12RHI, std::string("D3D12 Validation:") + std::string(pDescription));
				break;
			case D3D12_MESSAGE_SEVERITY_MESSAGE:
				VT_LOGC(Trace, LogD3D12RHI, std::string("D3D12 Validation:") + std::string(pDescription));
				break;
			default:
				break;
		}
	}

	D3D12DebugLayer::D3D12DebugLayer()
	{
		InitializeDebugLayer();
	}

	void D3D12DebugLayer::InitializeDebugLayer()
	{
		if (SUCCEEDED(D3D12GetDebugInterface(VT_D3D12_ID(m_debugInterface))))
		{
			m_isSupported = true;

			m_debugInterface->EnableDebugLayer();
		}
	}

	D3D12DebugLayer::~D3D12DebugLayer()
	{
		if (m_infoQueue)
		{
			m_infoQueue->UnregisterMessageCallback(m_debugCallbackId);
		}

		m_infoQueue = nullptr;
		m_debugInterface = nullptr;
	}

	void D3D12DebugLayer::InitializeAPIValidation(RawPtr<GraphicsDevice> graphicsDevice)
	{
		ID3D12Device10* d3d12Device = graphicsDevice->As<D3D12GraphicsDevice>()->GetDevice10();

		HRESULT result = d3d12Device->QueryInterface(m_infoQueue.ReleaseAndGetAddressOf());
		if (SUCCEEDED(result))
		{
			m_infoQueue->RegisterMessageCallback(&LogD3D12Message, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_debugCallbackId);
			m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
			m_infoQueue->SetBreakOnCategory(D3D12_MESSAGE_CATEGORY_CLEANUP, true);

			// #TODO_D3D12: Disabled for now
#if 1
			D3D12_MESSAGE_SEVERITY severities[] =
			{
				D3D12_MESSAGE_SEVERITY_INFO
			};

			D3D12_MESSAGE_ID denyIDs[] =
			{
				D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_NON_OPTIMAL_BARRIER_ONLY_EXECUTE_COMMAND_LISTS
			};

			D3D12_INFO_QUEUE_FILTER queueFilter{};
			queueFilter.DenyList.NumCategories = 0;
			queueFilter.DenyList.NumSeverities = 1;
			queueFilter.DenyList.pSeverityList = severities;
			queueFilter.DenyList.NumIDs = 3;
			queueFilter.DenyList.pIDList = denyIDs;

			VT_D3D12_CHECK(m_infoQueue->PushStorageFilter(&queueFilter));
#endif
		}
	}
}
