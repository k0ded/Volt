#pragma once

#include "WindowModule/WindowHandle.h"
#include "WindowModule/WindowProperties.h"

#include "WindowModule/Config.h"

#include <EventSystem/EventListener.h>

#include <SubSystem/SubSystem.h>
#include <SubSystem/SubSystemRegistry.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/Unique.h>

#include <unordered_map>

struct GLFWmonitor;

namespace Volt
{
	class Window;
	class Monitor;

	class WINDOWMODULE_API WindowManager : public SubSystem, public EventListener
	{
	public:
		WindowManager();
		~WindowManager();

		WindowManager(const WindowManager&) = delete;
		WindowManager& operator=(const WindowManager&) = delete;

		void Initialize() override;
		void Shutdown() override;

		void CreateMainWindow(const WindowProperties& windowProperties);
		void DestroyMainWindow();

		const WindowHandle CreateNewWindow(const WindowProperties& windowProperties);
		void DestroyWindow(const WindowHandle handle);
		void DestroyWindow(Window& window);

		void BeginFrame();
		void Render(float timestep);
		void Present();

		WindowHandle GetMainWindowHandle() const;

		Window& GetMainWindow() const;
		Window& GetWindow(const WindowHandle handle) const;

		VT_NODISCARD VT_INLINE bool HasMainWindow() const { return m_mainWindowHandle != 0; }
		VT_NODISCARD VT_INLINE const Vector<Ref<Monitor>>& GetMonitors() const { return m_monitors; }
		VT_NODISCARD VT_INLINE float GetPeakNits() const { return m_peakNits; }
		VT_INLINE void SetForceSDR(bool state) { m_forceSDR = state; }
		VT_INLINE void SetPeakNits(float peakNits) { m_peakNits = peakNits; }

		static WindowManager& Get();

		static void GetSubSystemDependencies(SubSystemDependencyList& outDependencies);
		VT_DECLARE_SUBSYSTEM("{DD8C1066-AA16-40C6-929D-282F15D11AC2}"_guid);

	private:
		inline static WindowManager* s_instance = nullptr;

		void InitializeMonitors();
		void InitializeGLFW();
		void ShutdownGLFW();

		Ref<Monitor> TryGetMonitor(GLFWmonitor* nativeMonitor);
		Ref<Monitor> AddMonitor(GLFWmonitor* nativeMonitor);
		void RemoveMonitor(Ref<Monitor> monitor);

		bool m_forceSDR = false;
		float m_peakNits = 250.f;

		WindowHandle m_mainWindowHandle = 0;
		std::unordered_map<WindowHandle, Unique<Window>> m_windows;

		Vector<Ref<Monitor>> m_monitors;
	};
}
