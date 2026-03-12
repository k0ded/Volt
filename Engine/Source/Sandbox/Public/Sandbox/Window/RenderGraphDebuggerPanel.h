#pragma once

#include "Sandbox/Window/EditorWindow.h"

namespace Volt
{
	class SceneRenderer;
}

class RenderGraphDebuggerPanel : public EditorWindow
{
public:
	RenderGraphDebuggerPanel(Ref<Volt::SceneRenderer>& sceneRenderer);
	void UpdateMainContent() override;
	void UpdateContent() override;

private:
	Ref<Volt::SceneRenderer>& m_sceneRenderer;
};
