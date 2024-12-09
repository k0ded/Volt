#include "RenderDocFrameCapture.h"

#include <CoreUtilities/EnumUtils.h>

#include <RHIModule/Graphics/GraphicsContext.h>
#include <RHIModule/Graphics/GraphicsDevice.h>

RenderDocFrameCapture::RenderDocFrameCapture(RENDERDOC_API_1_6_0* renderDocAPI)
	: m_renderDocAPI(renderDocAPI)
{
}

RenderDocFrameCapture::~RenderDocFrameCapture()
{
}

void RenderDocFrameCapture::StartFrameCapture()
{
	m_renderDocAPI->StartFrameCapture(Volt::RHI::GraphicsContext::GetDevice()->GetHandle<void*>(), nullptr);
}

void RenderDocFrameCapture::EndFrameCapture()
{
	m_renderDocAPI->EndFrameCapture(Volt::RHI::GraphicsContext::GetDevice()->GetHandle<void*>(), nullptr);
}

bool RenderDocFrameCapture::IsCaptureApplicationRunning() const
{
	return m_renderDocAPI->ShowReplayUI() != 0;
}

void RenderDocFrameCapture::LaunchCaptureApplication()
{
	m_renderDocAPI->LaunchReplayUI(1, nullptr);
}

void RenderDocFrameCapture::SetCaptureFileTargetFilePath(const std::filesystem::path& filePath)
{
	const std::string strPath = filePath.string();
	m_renderDocAPI->SetCaptureFilePathTemplate(strPath.c_str());
}

void RenderDocFrameCapture::SetFlags(Volt::RHI::FrameCaptureFlags flags)
{
	if (EnumValueContainsFlag(flags, Volt::RHI::FrameCaptureFlags::DisableOverlay))
	{
		m_renderDocAPI->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, 0);
	}
	else
	{
		m_renderDocAPI->MaskOverlayBits(RENDERDOC_OverlayBits::eRENDERDOC_Overlay_None, 1);
	}
}
