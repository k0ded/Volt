#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Volt-Animation/Assets/Animation.h>

namespace Volt
{
	class Animation;
}

class AnimationEditorPanel : public EditorWindow
{
public:
	AnimationEditorPanel();
	~AnimationEditorPanel() override = default;

	void UpdateMainContent() override;
	void OpenAsset(AssetReference<Volt::Asset_New> asset) override;

	void OnOpen() override;
	void OnClose() override;

private:
	struct AddAnimEventData
	{
		uint32_t frame;
		std::string name;
	};

	void AddAnimationEventModal();

	AssetReference<Volt::Animation> m_animation;
	int32_t m_selectedKeyFrame = -1;
	AddAnimEventData m_addAnimEventData{};
};
