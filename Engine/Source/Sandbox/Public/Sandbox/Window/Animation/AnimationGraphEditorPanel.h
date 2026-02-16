#pragma once
#include "Sandbox/Window/EditorWindow.h"

#include "Sandbox/NodeGraph/EditorNodeGraph.h"

#include <NodeGraph/NodeGraphBase.h>

namespace Volt
{
	class AnimationGraph;
}

class AnimationGraphEditorPanel final: public EditorWindow
{
public:
	AnimationGraphEditorPanel();
	void UpdateMainContent() override;

	void OpenAsset(AssetReference<Volt::Asset> asset) override;
private:
	Ref<NodeGraphBase> m_openAnimationGraph;
	
	EditorNodeGraph m_editorNodeGraph;
};
