#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeGraph.h"

#include <imgui.h>


EditorNodeGraph::EditorNodeGraph(std::string_view imGuiID)
	: m_imGuiID(imGuiID)
	, m_cameraPos(0, 0), m_movingCamera(false)
	, m_zoom(1)
	, m_graphScreenAreaTL(0, 0), m_graphScreenAreaBR(0, 0)
	, m_graphScreenSize(0,0)
	, m_graphVisibleWorldSize(0,0)

{
}

void EditorNodeGraph::Draw()
{

	ImGui::PushID(m_imGuiID.c_str());
	if (ImGui::BeginChild("NodeGraph"))
	{
		m_graphScreenSize = ImGui::GetWindowSize();
		m_graphScreenAreaTL = ImGui::GetWindowPos();
		m_graphScreenAreaBR = m_graphScreenAreaTL + m_graphScreenSize;
		m_graphVisibleWorldSize = m_graphScreenSize / m_zoom;

		DrawGraph();
	}
	ImGui::EndChild();
	ImGui::PopID();
}

NodeInstanceID EditorNodeGraph::SpawnNodeOfType(VoltGUID typeID)
{
	if (!m_registeredNodeTypes.contains(typeID))
	{
		VT_ENSURE_MSG(m_registeredNodeTypes.contains(typeID), "Tried to spawn a node type that doesnt exist!");
		return 0;
	}

	NodeTypeInfo& typeInfo = m_registeredNodeTypes[typeID];
	Ref<EditorNodeTypeBase> newNode = typeInfo.CreateInstanceFunc();
	m_spawnedNodes.push_back(newNode);

	//todo: return proper instance ID
	return 1;
}

void EditorNodeGraph::DrawGraph()
{
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		if (!m_movingCamera)
		{
			m_startMovingCameraPos = m_cameraPos;
		}
		m_movingCamera = true;

		m_cameraPos = m_startMovingCameraPos - ImGui::GetMouseDragDelta(ImGuiMouseButton_Right) * (1.f / m_zoom);
	}

	float scaleMul = 1.f * (1 + ImGui::GetIO().MouseWheel * 0.1f);
	m_zoom *= scaleMul;

	DrawGrid();
}

void EditorNodeGraph::DrawGrid()
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	for (float x = m_cameraPos.x - std::floor(m_graphVisibleWorldSize.x / 20.f) * 10.f; x < m_cameraPos.x + m_graphVisibleWorldSize.x; x += 10.f)
	{
		float lineWorldX = x - (std::fmod(m_cameraPos.x, 10.f));

 		const float screenX = WorldToScreenXPos(lineWorldX);
		if (screenX <= m_graphScreenAreaTL.x &&
			screenX >= m_graphScreenAreaBR.x)
		{
			return;
		}
		const ImVec2 p1 = { screenX, m_graphScreenAreaTL.y };
		const ImVec2 p2 = { screenX, m_graphScreenAreaBR.y};
		drawList->AddLine(p1, p2, GRID_LINE_COLOR, GRID_LINE_THICKNESS);
	}

	for (float y = m_cameraPos.y - std::floor(m_graphVisibleWorldSize.y / 20.f) * 10.f; y < m_cameraPos.y + m_graphVisibleWorldSize.y; y += 10.f)
	{
		float lineWorldY = y - (std::fmod(m_cameraPos.y, 10.f));
	
		const float screenY = WorldToScreenYPos(lineWorldY);
		if (screenY <= m_graphScreenAreaTL.y ||
			screenY >= m_graphScreenAreaBR.y)
		{
			return;
		}
		const ImVec2 p1 = { m_graphScreenAreaTL.x , screenY };
		const ImVec2 p2 = { m_graphScreenAreaBR.x ,screenY };
		drawList->AddLine(p1, p2, GRID_LINE_COLOR, GRID_LINE_THICKNESS);
	}
}

glm::vec2 EditorNodeGraph::SceenToWorldPos(const glm::vec2& screenPos)
{
	return (screenPos - m_graphScreenAreaTL - (m_graphScreenSize / 2.f)) + m_cameraPos;
}

float EditorNodeGraph::SceenToWorldXPos(float screenPos)
{
	return (screenPos - m_graphScreenAreaTL.x - (m_graphScreenSize.x / 2.f)) + m_cameraPos.x;
}

float EditorNodeGraph::SceenToWorldYPos(float screenPos)
{
	return (screenPos - m_graphScreenAreaTL.y - (m_graphScreenSize.y / 2.f)) + m_cameraPos.y;
}

glm::vec2 EditorNodeGraph::WorldToScreenPos(const glm::vec2& worldPos)
{
	return (worldPos - m_cameraPos) * m_zoom + m_graphScreenAreaTL + (m_graphScreenSize / 2.f);
}

float EditorNodeGraph::WorldToScreenXPos(float worldPos)
{
	return (worldPos - m_cameraPos.x) * m_zoom + m_graphScreenAreaTL.x + (m_graphScreenSize.x / 2.f);
}

float EditorNodeGraph::WorldToScreenYPos(float worldPos)
{
	return (worldPos - m_cameraPos.y) * m_zoom + m_graphScreenAreaTL.y + (m_graphScreenSize.y / 2.f);
}
