#pragma once
#include "Circuit/Config.h"

#include <EventSystem/EventListener.h>
#include <WindowModule/WindowHandle.h>

#include <CoreUtilities/Pointers/Unique.h>
#include <CoreUtilities/Pointers/Ref.h>
#include <CoreUtilities/Containers/Vector.h>

#include <map>

namespace Volt
{
	class WindowRenderEvent_New;
	class WindowCloseEvent_New;

	class Window;
}

namespace Circuit
{
	class CircuitInputHandler;
	class CircuitWindow;
	class Widget;


	class CircuitManager : Volt::EventListener
	{
	public:
		CircuitManager();
		~CircuitManager() = default;

		CIRCUIT_API static CircuitManager& Get();
		CIRCUIT_API static void Initialize();
		CIRCUIT_API static void Shutdown();

		CIRCUIT_API void Update();

		CIRCUIT_API Vector<Weak<CircuitWindow>> GetWindows();

		CIRCUIT_API Weak<CircuitWindow> CreateWindow(Ref<Widget> contentWidget);



		//CIRCUIT_API CircuitWindow& OpenWindow(OpenWindowParams& params);  
	private:
		CIRCUIT_API inline static Unique<CircuitManager> s_Instance = nullptr;

		void Init();

		static int32_t TestingStaticDelegates(float aParameter);
		int32_t TestingRawDelegates(float aParameter);

	private:
		std::map<Volt::WindowHandle, Ref<CircuitWindow>> m_windows;

		Unique<CircuitInputHandler> InputHandler;
	};
}
