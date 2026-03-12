#pragma once
#include "Circuit/Config.h"

#include <EventSystem/EventListener.h>
#include <WindowModule/WindowHandle.h>

#include <map>


namespace Volt
{
	class WindowRenderEvent;

	class Window;
}

namespace Circuit
{
	class CircuitInputHandler;
	class CircuitWindow;


	class CircuitManager : Volt::EventListener
	{
	public:
		CircuitManager();
		~CircuitManager() = default;

		CIRCUIT_API static CircuitManager& Get();
		CIRCUIT_API static void Initialize();

		CIRCUIT_API void Update();

		CIRCUIT_API Vector<Weak<CircuitWindow>> GetWindows();

		//CIRCUIT_API CircuitWindow& OpenWindow(OpenWindowParams& params);  
	private:
		CIRCUIT_API inline static std::unique_ptr<CircuitManager> s_Instance = nullptr;

		void Init();
		void RegisterEventListeners();

		bool OnRenderEvent(Volt::WindowRenderEvent& e);

		void RegisterWindow(Volt::WindowHandle handle);

		static int32_t TestingStaticDelegates(float aParameter);
		int32_t TestingRawDelegates(float aParameter);

	private:
		std::map<Volt::WindowHandle, Ref<CircuitWindow>> m_windows;

		Scope<CircuitInputHandler> InputHandler;
	};
}
