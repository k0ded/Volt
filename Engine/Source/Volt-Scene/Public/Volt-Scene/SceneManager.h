#pragma once

#include "Volt-Scene/Config.h"

#include <CoreUtilities/Weak.h>

namespace Volt
{
	class Scene;
	class VTS_API SceneManager
	{
	public:
		static void Shutdown();
		inline static void SetActiveScene(Weak<Scene> scene) { m_activeScene = scene; }
		inline static Weak<Scene> GetActiveScene() { return m_activeScene; }
		static bool IsPlaying();

	private:
		SceneManager() = delete;
		inline static Weak<Scene> m_activeScene;
	};
}
