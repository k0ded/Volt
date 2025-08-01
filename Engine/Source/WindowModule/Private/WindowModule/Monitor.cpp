#include "windowpch.h"
#include "WindowModule/Monitor.h"
#include "WindowModule/WindowLogCategory.h"

#include <LogModule/Log.h>

#include <GLFW/glfw3.h>

namespace Volt
{
	Monitor::Monitor(GLFWmonitor* nativeMonitor)
		: m_nativeMonitor(nativeMonitor)
	{
		Initialize();

		std::stringstream sstream;
		sstream << "Initialized monitor: " << m_monitorName << "\n";
		sstream << "	Num Video Modes: " << m_videoModes.size() << "\n";
		sstream << "	Size: " << m_monitorSize.x << ", " << m_monitorSize.y << "\n";

		VT_LOGC_UNFORMATTED(Trace, LogWindowManagement, sstream.str());
	}

	void Monitor::Initialize()
	{
		// Monitor properties
		{
			glfwGetMonitorPos(m_nativeMonitor, &m_monitorPos.x, &m_monitorPos.y);
			glfwGetMonitorWorkarea(m_nativeMonitor, &m_monitorWorkPos.x, &m_monitorWorkPos.y, &m_monitorWorkSize.x, &m_monitorWorkSize.y);
			glfwGetMonitorContentScale(m_nativeMonitor, &m_monitorContentScale.x, &m_monitorContentScale.y);
		
			m_monitorName = glfwGetMonitorName(m_nativeMonitor);
		}

		// Find view modes
		{
			const GLFWvidmode* primaryVideoMode = glfwGetVideoMode(m_nativeMonitor);

			int32_t numVideoModes;
			const GLFWvidmode* modes = glfwGetVideoModes(m_nativeMonitor, &numVideoModes);

			m_videoModes.resize(numVideoModes);

			for (int32_t i = 0; i < numVideoModes; ++i)
			{
				const bool isPrimaryViewMode =
					modes[i].width == primaryVideoMode->width &&
					modes[i].height == primaryVideoMode->height &&
					modes[i].redBits == primaryVideoMode->redBits &&
					modes[i].greenBits == primaryVideoMode->greenBits &&
					modes[i].blueBits == primaryVideoMode->blueBits &&
					modes[i].refreshRate == primaryVideoMode->refreshRate;

				if (isPrimaryViewMode)
				{
					m_primaryVideoModeIndex = i;
					m_monitorSize = { modes[i].width, modes[i].height };
				}

				m_videoModes[i].width = modes[i].width;
				m_videoModes[i].height = modes[i].height;
				m_videoModes[i].numRedBits = modes[i].redBits;
				m_videoModes[i].numGreenBits = modes[i].greenBits;
				m_videoModes[i].numBlueBits = modes[i].blueBits;
				m_videoModes[i].refreshRate = modes[i].refreshRate;
			}
		}
	}
}
