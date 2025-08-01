#pragma once

#include <CoreUtilities/Containers/Vector.h>

#include <glm/glm.hpp>

#include <cstdint>

struct GLFWmonitor;

namespace Volt
{
	struct VideoMode
	{
		int32_t width;
		int32_t height;
		int32_t numRedBits;
		int32_t numGreenBits;
		int32_t numBlueBits;
		int32_t refreshRate;
	};

	class Monitor
	{
	public:
		Monitor(GLFWmonitor* nativeMonitor);

		VT_NODISCARD VT_INLINE GLFWmonitor* GetNativeMonitor() const { return m_nativeMonitor; }

		VT_NODISCARD VT_INLINE const glm::ivec2& GetMonitorPos() const { return m_monitorPos; }
		VT_NODISCARD VT_INLINE const glm::ivec2& GetMonitorSize() const { return m_monitorSize; }
		VT_NODISCARD VT_INLINE const glm::ivec2& GetMonitorWorkPos() const { return m_monitorWorkPos; }
		VT_NODISCARD VT_INLINE const glm::ivec2& GetMonitorWorkSize() const { return m_monitorWorkSize; }
		VT_NODISCARD VT_INLINE const glm::vec2& GetContentScale() const { return m_monitorContentScale; }

	private:
		void Initialize();

		glm::ivec2 m_monitorPos;
		glm::ivec2 m_monitorSize;
		glm::ivec2 m_monitorWorkPos;
		glm::ivec2 m_monitorWorkSize;

		glm::vec2 m_monitorContentScale;

		int32_t m_primaryVideoModeIndex = 0;
		std::string_view m_monitorName;

		GLFWmonitor* m_nativeMonitor;
		Vector<VideoMode> m_videoModes;
	};
}
