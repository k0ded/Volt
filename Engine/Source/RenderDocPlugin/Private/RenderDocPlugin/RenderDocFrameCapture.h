#pragma once

#include <RHIModule/FrameCapture.h>

#include <RenderDoc/renderdoc_app.h>

class RenderDocFrameCapture : public Volt::RHI::FrameCapture
{
public:
	RenderDocFrameCapture(RENDERDOC_API_1_6_0* renderDocAPI);
	~RenderDocFrameCapture() override;

	void StartFrameCapture() override;
	void EndFrameCapture() override;

	bool IsCaptureApplicationRunning() const override;
	void LaunchCaptureApplication() override;

	void SetCaptureFileTargetFilePath(const std::filesystem::path& filePath) override;
	void SetFlags(Volt::RHI::FrameCaptureFlags flags) override;

private:
	RENDERDOC_API_1_6_0* m_renderDocAPI;
};
