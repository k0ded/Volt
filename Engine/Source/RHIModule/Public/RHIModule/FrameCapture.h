#pragma once

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <cstdint>

namespace Volt::RHI
{
	enum class FrameCaptureFlags : uint8_t
	{
		None = 0,
		DisableOverlay = 1,
	};

	VT_SETUP_ENUM_CLASS_OPERATORS(FrameCaptureFlags);

	class FrameCapture
	{
	public:
		virtual ~FrameCapture() = default;

		virtual void StartFrameCapture() = 0;
		virtual void EndFrameCapture() = 0;

		virtual bool IsCaptureApplicationRunning() const = 0;
		virtual void LaunchCaptureApplication() = 0;

		virtual void SetCaptureFileTargetFilePath(const Filesystem::Path& filePath) = 0;
		virtual void SetFlags(FrameCaptureFlags flags) = 0;

	protected:
		FrameCapture() = default;
	};
}
