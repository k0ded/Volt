#pragma once

#include <EventSystem/EventListener.h>

#include <CoreUtilities/Containers/Map.h>

#include <glm/glm.hpp>

struct ImGuiContext;
struct ImGuiViewport;

namespace Volt
{
	class Window;
	class ImGuiPlatform : public EventListener
	{
	public:
		ImGuiPlatform();
		~ImGuiPlatform();

		void Destroy();

		void BeginFrame();

		void AddViewportWindow(Window* window);
		void RemoveViewportWindow(Window* window);

	private:
		struct ContextData
		{
			ImGuiContext* imguiContext;
			Window* mouseWindow = nullptr;
			glm::vec2 lastValidMousePos;
		};

		void RegisterEventListeners();
		void Initialize();
		void InitializeMonitors();

		void UpdateMouseData();
		void UpdateMouseCursor();

		void InitializeViewportSupport();

		ContextData& GetContextDataFromWindow(Window& window);
		ImGuiContext* GetImGuiContextFromWindow(Window& window);

		Map<Window*, ContextData> m_windowToContextMap;
		float m_deltaTime = 0.f;
	};
}
