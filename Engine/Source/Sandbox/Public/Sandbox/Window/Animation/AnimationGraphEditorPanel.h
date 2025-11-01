#pragma once
#include "Sandbox/Window/EditorWindow.h"

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
	Ref<Volt::AnimationGraph> m_openAnimationGraph;

	glm::vec2 m_cameraPos;
	float m_cameraZoom;

	bool m_movingCamera;
	glm::vec2 m_startMovingCameraPos;
};
