#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeGraph.h"

#include <imgui.h>
#include <imgui_internal.h>


EditorNodeGraph::EditorNodeGraph(std::string_view imGuiID)
	: m_imGuiID(imGuiID)
	, m_cameraPos(0, 0), m_movingCamera(false)
	, m_zoom(1)
	, m_graphScreenAreaTL(0, 0), m_graphScreenAreaBR(0, 0)
	, m_graphScreenSize(0, 0)
	, m_graphVisibleWorldSize(0, 0)

{
	RegisterNodeType<NothingNode>();
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

		UserNodeHandling();
		UserCameraHandling();

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

	const NodeTypeInfo& typeInfo = m_registeredNodeTypes[typeID];

	const NodeInstanceID newInstanceID = m_nextNodeInstanceID++;
	NodeInstanceInfo info;
	info.instanceID = newInstanceID;
	info.nodeInstance = typeInfo.createInstanceFunc();
	info.position = ScreenToWorldPos(ImGui::GetMousePos());
	m_nodeInstances.insert({ info.instanceID, std::move(info) });

	return newInstanceID;
}

bool EditorNodeGraph::IsNodeSelected(NodeInstanceID instanceID) const
{
	return m_selectedNodes.contains(instanceID);
}

bool EditorNodeGraph::IsNodeHovered(NodeInstanceID instanceID) const
{
	return instanceID == m_hoveredNode;
}

void EditorNodeGraph::DrawGraph()
{
	DrawGrid();

	for (auto& [instanceID, nodeInstanceInfo] : m_nodeInstances)
	{
		DrawNode(nodeInstanceInfo);
	}

	if (!m_lastFrameMovingCamera)
	{
		constexpr const char* GRAPH_POPUP_ID = "GRAPH_TOOLMENU";
		constexpr const char* NODE_POPUP_ID = "NODE_TOOLMENU";

		const std::string graphPopupID = GRAPH_POPUP_ID + m_imGuiID;
		const std::string nodePopupID = NODE_POPUP_ID + m_imGuiID;

		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			if (m_hoveredNode != 0)
			{
				m_toolMenuPopupNode = m_hoveredNode;
				ImGui::OpenPopup(nodePopupID.c_str());
			}
			else
			{
				ImGui::OpenPopup(graphPopupID.c_str());
			}
		}

		if (ImGui::BeginPopup(nodePopupID.c_str()))
		{
			if (ImGui::MenuItem("Delete"))
			{
				m_nodeInstances.erase(m_toolMenuPopupNode);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup(graphPopupID.c_str()))
		{
			if (ImGui::MenuItem("New Node"))
			{
				SpawnNodeOfType<NothingNode>();
			}
			ImGui::EndPopup();
		}
	}
}

void EditorNodeGraph::DrawNodeContent(const NodeInstanceInfo& nodeInstanceInfo, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax) const
{
	//nodeInstanceInfo.nodeInstance->
}

void EditorNodeGraph::DrawNode(const NodeInstanceInfo& nodeInstanceInfo)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	constexpr ImColor NODE_COLOR = 0xffffffff;
	constexpr float NODE_HOVERED_COLOR_MULTIPLIER = 0.8f;
	constexpr ImColor NODE_SELECTED_OUTLINE_COLOR = ImColor(1.f, 0.647f, 0.f);


	constexpr float NODE_EDGE_ROUNDING = 3.f;
	constexpr float NODE_WIDTH = 100;
	constexpr float NODE_HEIGHT = 40;
	constexpr ImVec2 NODE_SIZE(NODE_WIDTH, NODE_HEIGHT);
	constexpr ImVec2 HALF_NODE_SIZE(NODE_WIDTH / 2, NODE_HEIGHT / 2);

	const glm::vec2 halfNodeScreenSize = HALF_NODE_SIZE * m_zoom;
	const glm::vec2 nodeScreenPos = WorldToScreenPos(nodeInstanceInfo.position);
	const glm::vec2 nodeScreenMin = nodeScreenPos - halfNodeScreenSize;
	const glm::vec2 nodeScreenMax = nodeScreenPos + halfNodeScreenSize;
	//if the node isnt visible, dont draw it
	if (nodeScreenMax.x < m_graphScreenAreaTL.x ||
		nodeScreenMax.y < m_graphScreenAreaTL.y ||
		nodeScreenMin.x > m_graphScreenAreaBR.x ||
		nodeScreenMin.y > m_graphScreenAreaBR.y)
	{
		return;
	}


	GraphManageNode(nodeInstanceInfo, nodeScreenMin, nodeScreenMax);
	ImColor nodeColor = NODE_COLOR;
	if (IsNodeSelected(nodeInstanceInfo.instanceID))
	{
		drawList->AddRect(nodeScreenMin - ImVec2(3, 3), nodeScreenMax + ImVec2(3, 3), NODE_SELECTED_OUTLINE_COLOR, NODE_EDGE_ROUNDING * m_zoom, 0, 3.f);
	}
	if (IsNodeHovered(nodeInstanceInfo.instanceID))
	{
		nodeColor.Value.x *= NODE_HOVERED_COLOR_MULTIPLIER;
		nodeColor.Value.y *= NODE_HOVERED_COLOR_MULTIPLIER;
		nodeColor.Value.z *= NODE_HOVERED_COLOR_MULTIPLIER;
	}
	drawList->AddRectFilled(nodeScreenMin, nodeScreenMax, nodeColor, NODE_EDGE_ROUNDING * m_zoom);

	glm::vec2 nodeContentRectMin = nodeScreenMin;
	glm::vec2 nodeContentRectMax = nodeScreenMax;
	DrawNodeHeader(nodeInstanceInfo, nodeContentRectMin, nodeContentRectMax);

	DrawNodeContent(nodeInstanceInfo, nodeContentRectMin, nodeContentRectMax);
}

void EditorNodeGraph::DrawNodeHeader(const NodeInstanceInfo& nodeInstanceInfo, glm::vec2& minScreenPos, glm::vec2& maxScreenPos) const
{
	//default is to draw the typename at the top of the node

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	constexpr float HEADER_HEIGHT = 14.f;
	constexpr float HEADER_TEXT_PADDING = 2.f;
	constexpr float HEADER_FONT_SIZE = 10.f;
	constexpr ImColor HEADER_BACKGROUND_COLOR = ImColor(0.5f, 0.32f, 0.f, 0.7f);

	const float headerHeight = HEADER_HEIGHT * m_zoom;
	glm::vec2 headerMaxPos = { maxScreenPos.x, minScreenPos.y + headerHeight };
	drawList->AddRectFilled(minScreenPos, headerMaxPos, HEADER_BACKGROUND_COLOR, 3.f * m_zoom, ImDrawFlags_RoundCornersTop);

	const float fontSize = HEADER_FONT_SIZE * m_zoom;
	const float textPadding = HEADER_TEXT_PADDING * m_zoom;
	drawList->AddText(ImGui::GetFont(), fontSize, minScreenPos + glm::vec2(textPadding, textPadding), 0xffffffff, nodeInstanceInfo.nodeInstance->GetTypeName().c_str());

	//modify the content bounds to account for the header
	minScreenPos.y += headerHeight;
}

void EditorNodeGraph::DrawConnection(const NodeConnection& nodeConnection) const
{}

void EditorNodeGraph::GraphManageNode(const NodeInstanceInfo& nodeInstanceInfo, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos)
{
	ImGui::ItemAdd({ minScreenPos, maxScreenPos }, nodeInstanceInfo.instanceID);
	if (ImGui::IsItemHovered())
	{
		m_hoveredNode = nodeInstanceInfo.instanceID;
	}
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
		const ImVec2 p2 = { screenX, m_graphScreenAreaBR.y };
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

glm::vec2 EditorNodeGraph::ScreenToWorldPos(const glm::vec2& screenPos) const
{
	return (screenPos - m_graphScreenAreaTL - (m_graphScreenSize / 2.f)) + m_cameraPos;
}

float EditorNodeGraph::ScreenToWorldXPos(float screenPos) const
{
	return (screenPos - m_graphScreenAreaTL.x - (m_graphScreenSize.x / 2.f)) + m_cameraPos.x;
}

float EditorNodeGraph::ScreenToWorldYPos(float screenPos) const
{
	return (screenPos - m_graphScreenAreaTL.y - (m_graphScreenSize.y / 2.f)) + m_cameraPos.y;
}

glm::vec2 EditorNodeGraph::WorldToScreenPos(const glm::vec2& worldPos) const
{
	return (worldPos - m_cameraPos) * m_zoom + m_graphScreenAreaTL + (m_graphScreenSize / 2.f);
}

float EditorNodeGraph::WorldToScreenXPos(float worldPos) const
{
	return (worldPos - m_cameraPos.x) * m_zoom + m_graphScreenAreaTL.x + (m_graphScreenSize.x / 2.f);
}

float EditorNodeGraph::WorldToScreenYPos(float worldPos) const
{
	return (worldPos - m_cameraPos.y) * m_zoom + m_graphScreenAreaTL.y + (m_graphScreenSize.y / 2.f);
}

void EditorNodeGraph::UserNodeHandling()
{
	if (m_hoveredNode != 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		m_holdingNode = m_hoveredNode;
		m_selectedNodes.clear();
		m_selectedNodes.insert(m_holdingNode);
	}
	//reset hovered node and let it be reevaluated
	m_hoveredNode = 0;

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (m_holdingNode != 0)
		{
			m_holdingNode = 0;
		}
		m_draggingNodes = false;
		m_startDraggingNodePositions.clear();
	}
	//if dragging any node, user wants to drag all the selected ones
	if (!m_draggingNodes && m_holdingNode && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
	{
		m_draggingNodes = true;
		//remember where selected nodes were when user started dragging
		for (const NodeInstanceID& selectedNodeID : m_selectedNodes)
		{
			m_startDraggingNodePositions.insert({ selectedNodeID, m_nodeInstances.at(selectedNodeID).position });
		}
	}
	if (m_draggingNodes)
	{
		const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) * (1.f / m_zoom);
		for (const NodeInstanceID& selectedNodeID : m_selectedNodes)
		{
			NodeInstanceInfo& instanceInfo = m_nodeInstances[selectedNodeID];
			instanceInfo.position = m_startDraggingNodePositions[selectedNodeID] + delta;
		}
	}
}

void EditorNodeGraph::UserCameraHandling()
{
	m_lastFrameMovingCamera = m_movingCamera;
	if (ImGui::IsMouseDragging(ImGuiMouseButton_Right))
	{
		if (!m_movingCamera)
		{
			m_startMovingCameraPos = m_cameraPos;
		}
		m_movingCamera = true;

		m_cameraPos = m_startMovingCameraPos - ImGui::GetMouseDragDelta(ImGuiMouseButton_Right) * (1.f / m_zoom);
	}
	else
	{
		m_movingCamera = false;
	}

	float scaleMul = 1.f * (1 + ImGui::GetIO().MouseWheel * 0.1f);
	m_zoom *= scaleMul;
}

