#include "sbpch.h"
#include "Sandbox/Window/Animation/AnimationGraphEditorPanel.h"

#include <Volt-Animation/Assets/AssetTypes.h>

#include <imgui.h>

AnimationGraphEditorPanel::AnimationGraphEditorPanel()
	: EditorWindow("Animation Graph Editor")
	, m_editorNodeGraph("Animation Node Graph")
{
	m_openAnimationGraph = CreateRef<NodeGraphBase>();
}

void AnimationGraphEditorPanel::UpdateMainContent()
{
	m_editorNodeGraph.Draw(*m_openAnimationGraph);
}

void AnimationGraphEditorPanel::OpenAsset(AssetReference<Volt::Asset> asset)
{
	VT_ENSURE(asset->GetType() == AssetTypes::AnimationGraph);
}
