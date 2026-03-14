#include "Plugin.h"

#include "RenderDocFrameCapture.h"

#include <Volt-Core/Console/ConsoleVariableRegistry.h>

#include <RHIModule/RHIModule.h>
#include <WindowModule/Events/WindowEvents.h>

#include <LogModule/Log.h>

#include <CoreUtilities/DynamicLibraryHelpers.h>

VT_DEFINE_LOG_CATEGORY(LogRenderDoc);

static Volt::ConsoleVariable<int32_t> s_rdcCaptureFrame("r.rdc.capture", 0, "Trigger a capture");

RenderDocPlugin::RenderDocPlugin()
{
	if (m_renderDocModule = VT_LOAD_LIBRARY("Binaries\\renderdoc.dll"); m_renderDocModule != nullptr)
	{
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(VT_GET_PROC_ADDRESS(m_renderDocModule, "RENDERDOC_GetAPI"));
		int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_6_0, reinterpret_cast<void**>(&m_renderDocAPI));

		if (result != 1)
		{
			m_renderDocAPI = nullptr;
			VT_FREE_LIBRARY(m_renderDocModule);
			m_renderDocModule = nullptr;

			VT_LOGC(Error, LogRenderDoc, "Failed to load RenderDoc API!");
		}
	}
}

RenderDocPlugin::~RenderDocPlugin()
{
	if (m_renderDocModule)
	{
		VT_FREE_LIBRARY(m_renderDocModule);
	}

	m_renderDocModule = nullptr;
}

void RenderDocPlugin::Initialize()
{
	if (m_renderDocAPI)
	{
		m_frameCapture = CreateRef<RenderDocFrameCapture>(m_renderDocAPI);
		m_frameCapture->SetFlags(Volt::RHI::FrameCaptureFlags::DisableOverlay);
		Volt::RHI::RHIModule::GetInstance().SetFrameCapture(m_frameCapture);

		m_eventListener = CreateScope<RenderDocEventListener>();
		m_eventListener->RegisterListeners(m_frameCapture);
	}
}

void RenderDocPlugin::Shutdown()
{
	m_eventListener = nullptr;
	m_frameCapture = nullptr;
}

RenderDocEventListener::~RenderDocEventListener()
{
}

void RenderDocEventListener::RegisterListeners(Ref<RenderDocFrameCapture> frameCapture)
{
	m_frameCapture = frameCapture;

	RegisterListener<Volt::WindowBeginFrameEvent>([&](Volt::WindowBeginFrameEvent& e) 
	{
		if (s_rdcCaptureFrame.GetValue() && m_frameCapture)
		{
			m_frameCapture->StartFrameCapture();
			s_rdcCaptureFrame.SetValue(0);

			m_isCapturing = true;
		}

		return false;
	});

	RegisterListener<Volt::WindowPresentFrameEvent>([&](Volt::WindowPresentFrameEvent& e)
	{
		if (m_isCapturing)
		{
			m_isCapturing = false;

			if (m_frameCapture)
			{
				m_frameCapture->EndFrameCapture();

				if (!m_frameCapture->IsCaptureApplicationRunning())
				{
					m_frameCapture->LaunchCaptureApplication();
				}
			}
		}

		return false;
	});
}
