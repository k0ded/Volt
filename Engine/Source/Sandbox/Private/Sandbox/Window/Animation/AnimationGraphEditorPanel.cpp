#include "sbpch.h"
#include "Sandbox/Window/Animation/AnimationGraphEditorPanel.h"

#include <Volt-Animation/Assets/AssetTypes.h>

#include <imgui.h>

AnimationGraphEditorPanel::AnimationGraphEditorPanel()
	: EditorWindow("Animation Graph Editor")
{
	m_cameraPos = { 0,0 };
	m_cameraZoom = 1.f;
	m_movingCamera = false;
}

void AnimationGraphEditorPanel::UpdateMainContent()
{
	constexpr float lineThickness = 1.f;
	constexpr ImU32 lineColor = 0x383030ff;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	ImVec2 gridScreenTopLeft = ImGui::GetWindowPos();
	ImVec2 gridScreenSize = ImGui::GetWindowSize();
	ImVec2 visibleWorldSize = gridScreenSize / m_cameraZoom;

	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		if (!m_movingCamera)
		{
			m_startMovingCameraPos = m_cameraPos;
		}
		m_movingCamera = true;
	}
	else
	{
		m_movingCamera = false;
	}

	if (m_movingCamera)
	{
		m_cameraPos = m_startMovingCameraPos - ImGui::GetMouseDragDelta(ImGuiMouseButton_Right) * (1.f / m_cameraZoom);
	}
	float scaleMul = 1.f * (1 + ImGui::GetIO().MouseWheel * 0.1f);
	m_cameraZoom *= scaleMul;

	auto worldToScreenPos = [&](const ImVec2& worldPos)
	{
		return (worldPos - m_cameraPos) * m_cameraZoom + gridScreenTopLeft + (gridScreenSize / 2.f);
	};
	auto worldToScreenXPos = [&](float worldPos)
	{
		return (worldPos - m_cameraPos.x) * m_cameraZoom + gridScreenTopLeft.x + (gridScreenSize.x / 2.f);
	};

	auto worldToScreenYPos = [&](float worldPos)
	{
		return (worldPos - m_cameraPos.y) * m_cameraZoom + gridScreenTopLeft.y + (gridScreenSize.y / 2.f);
	};

	auto screenToWorldPos = [&](const ImVec2& screenPos)
	{
		return (screenPos - gridScreenTopLeft - (visibleWorldSize / 2.f)) + m_cameraPos ;
	};

	auto drawHorizontalGridLine = [&](float worldY)
	{
		const float screenY = worldToScreenYPos(worldY);
		if (screenY <= gridScreenTopLeft.y ||
			screenY >= (gridScreenTopLeft.y + gridScreenSize.y))
		{
			return;
		}
		const ImVec2 p1 = { gridScreenTopLeft.x , screenY };
		const ImVec2 p2 = { gridScreenTopLeft.x + gridScreenSize.x ,screenY };
		drawList->AddLine(p1, p2, lineColor, lineThickness);
	};

	auto drawVerticalGridLine = [&](float worldX)
	{
		const float screenX = worldToScreenXPos(worldX);
		if (screenX <= gridScreenTopLeft.x &&
			screenX >= (gridScreenTopLeft.x + gridScreenSize.x))
		{
			return;
		}
		const ImVec2 p1 = { worldToScreenXPos(worldX), gridScreenTopLeft.y };
		const ImVec2 p2 = { worldToScreenXPos(worldX), gridScreenTopLeft.y + gridScreenSize.y };
		drawList->AddLine(p1, p2, lineColor, lineThickness);
	};

	for (float x = m_cameraPos.x - std::floor(visibleWorldSize.x/10.f) * 10.f; x < m_cameraPos.x + visibleWorldSize.x; x += 10.f)
	{
		float useX = x - (std::fmod(m_cameraPos.x, 10.f));
		drawVerticalGridLine(useX);
	}

	for (float y = m_cameraPos.y - std::floor(visibleWorldSize.x / 10.f) * 10.f; y < m_cameraPos.y + visibleWorldSize.y; y += 10.f)
	{
		float useY = y - (std::fmod(m_cameraPos.y, 10.f));
		drawHorizontalGridLine(useY);
	}

	drawList->AddCircleFilled(worldToScreenPos(m_cameraPos), 5, 0xff0000ff);


	drawList->AddText(gridScreenTopLeft + gridScreenSize / 2.f, 0xff0000ff, std::format("cameraPos: {0}, {1}", std::to_string(m_cameraPos.x), std::to_string(m_cameraPos.y)).c_str());
	drawList->AddText(gridScreenTopLeft + gridScreenSize / 2.f + ImVec2(0, ImGui::CalcTextSize("").y), 0xff0000ff, std::format("zoom: {0}", std::to_string(m_cameraZoom)).c_str());
}

void AnimationGraphEditorPanel::OpenAsset(AssetReference<Volt::Asset> asset)
{
	VT_ENSURE(asset->GetType() == AssetTypes::AnimationGraph);
}
