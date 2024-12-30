#include "sbpch.h"
#include "Sandbox/Window/RenderGraphDebuggerPanel.h"

#include <Volt/Utility/UIUtility.h>

#include <Volt/Rendering/SceneRenderer.h>

RenderGraphDebuggerPanel::RenderGraphDebuggerPanel(Ref<Volt::SceneRenderer>& sceneRenderer)
	: EditorWindow("Render Graph Debugger"), m_sceneRenderer(sceneRenderer)
{
}

void RenderGraphDebuggerPanel::UpdateMainContent()
{
	const auto& renderGraphDebugger = m_sceneRenderer->GetRenderGraphDebugger();

	renderGraphDebugger.WaitForFinishedExecution();
	const auto& images = renderGraphDebugger.GetImages();

	for (const auto& image : images)
	{
		if (image)
		{
			ImGui::Image(UI::GetTextureID(image), { 160.f, 90.f });
		}
	}
}
