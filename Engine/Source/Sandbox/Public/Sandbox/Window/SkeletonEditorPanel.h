#pragma once

#include "Sandbox/Window/EditorWindow.h"

#include <Volt-Animation/Assets/Skeleton.h>

namespace Volt
{
	class Skeleton;
}

class SkeletonEditorPanel : public EditorWindow
{
public:
	SkeletonEditorPanel();
	~SkeletonEditorPanel() override = default;

	void UpdateMainContent() override;
	void OpenAsset(AssetReference<Volt::Asset> asset) override;

	void OnOpen() override;
	void OnClose() override;

private:
	void AddJointAttachmentPopup();

	AssetReference<Volt::Skeleton> m_skeleton;

	bool m_activateJointSearch = false;
	std::string m_jointSearchQuery;
};
