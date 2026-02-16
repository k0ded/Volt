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
	memset(&m_style, 0, sizeof(m_style));
	SetupStyle(m_style);
}

void EditorNodeGraph::Draw(NodeGraphBase& NodeGraph)
{
	ImGui::PushID(m_imGuiID.c_str());
	if (ImGui::BeginChild("NodeGraph"))
	{
		m_graphScreenSize = ImGui::GetWindowSize();
		m_graphScreenAreaTL = ImGui::GetWindowPos();
		m_graphScreenAreaBR = m_graphScreenAreaTL + m_graphScreenSize;
		m_graphVisibleWorldSize = m_graphScreenSize / m_zoom;

		UserNodeHandling(NodeGraph);
		UserCameraHandling();

		DrawGraph(NodeGraph);
	}
	ImGui::EndChild();
	ImGui::PopID();
}

bool EditorNodeGraph::IsNodeSelected(NodeInstanceID instanceID) const
{
	return m_selectedNodes.contains(instanceID);
}

bool EditorNodeGraph::IsNodeHovered(NodeInstanceID instanceID) const
{
	return instanceID == m_hoveredNode;
}

void EditorNodeGraph::SetupStyle(Style& style)
{
	//setup defaults
	style.SetVar(StyleVar::Node_HeaderFontSize, 12.f);
	style.SetVar(StyleVar::Node_HeaderTextPadding, 2.f);

	style.SetVar(StyleVar::Node_EdgeRounding, 3.f);
	style.SetVar(StyleVar::Node_ContentPadding, 3.f);

	style.SetVar(StyleVar::Pin_Radius, 5.f);
	style.SetVar(StyleVar::Pin_VPadding, 5.f);
	style.SetVar(StyleVar::Pin_HPadding, 10.f);
	style.SetVar(StyleVar::Pin_UnconnectedThickness, 1.5f);
	style.SetVar(StyleVar::Pin_TextPadding, 5.f);
	style.SetVar(StyleVar::Pin_TextFontSize, 8.f);

	style.SetColor(StyleColor::Node_BgColor, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
	style.SetColor(StyleColor::Node_HoveredBgColor, glm::vec4(0.3f, 0.3f, 0.3f, 0.9f));

	style.SetColor(StyleColor::Node_HeaderBgColor, glm::vec4(0.5f, 0.32f, 0.f, 1.f));
	style.SetColor(StyleColor::Node_HoveredHeaderBgColor, glm::vec4(0.6f, 0.42f, 0.1f, 1.f));

	style.SetColor(StyleColor::Node_SelectedOutlineColor, glm::vec4(1.f, 0.647f, 0.f, 1.f));
}

void EditorNodeGraph::DrawGraph(NodeGraphBase& NodeGraph)
{
	DrawGrid();

	for (auto& [instanceID, instance] : NodeGraph.GetNodeInstancesMap())
	{
		DrawNode(NodeGraph.GetMutableNodeInstance(instanceID));
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
				NodeGraph.DeleteNode(m_toolMenuPopupNode);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup(graphPopupID.c_str()))
		{
			if (ImGui::MenuItem("New Node"))
			{
				NodeGraph.SpawnNodeOfType<NothingNode>();
			}
			ImGui::EndPopup();
		}
	}
}
void EditorNodeGraph::DrawNode(NodeInstance& instance)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 nodeSize = instance.desiredSize;
	const ImVec2 halfNodeSize = nodeSize / 2.f;

	const glm::vec2 halfNodeScreenSize = halfNodeSize * m_zoom;
	const glm::vec2 nodeScreenPos = WorldToScreenPos(instance.position);
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

	GraphManageNode(instance, nodeScreenMin, nodeScreenMax);
	if (IsNodeSelected(instance.instanceID))
	{
		drawList->AddRect(nodeScreenMin - ImVec2(3, 3), nodeScreenMax + ImVec2(3, 3), ImColor(GetStyleColor(StyleColor::Node_SelectedOutlineColor)), GetScaledStyleVar(StyleVar::Node_EdgeRounding), 0, 3.f);
	}

	ImColor nodeColor = {};
	if (IsNodeHovered(instance.instanceID))
	{
		nodeColor = ImColor(GetStyleColor(StyleColor::Node_HoveredBgColor));
	}
	else
	{
		nodeColor = ImColor(GetStyleColor(StyleColor::Node_BgColor));
	}

	drawList->AddRectFilled(nodeScreenMin, nodeScreenMax, nodeColor, GetScaledStyleVar(StyleVar::Node_EdgeRounding));

	glm::vec2 desiredHeaderSize;
	DrawNodeHeader(instance, nodeScreenMin, nodeScreenMax, desiredHeaderSize);

	glm::vec2 desiredContentSize;
	DrawNodeContent(instance, nodeScreenMin + ImVec2(0, desiredHeaderSize.y), nodeScreenMax, desiredContentSize);

	instance.desiredSize.x = glm::max(desiredHeaderSize.x, desiredContentSize.x) / m_zoom;
	instance.desiredSize.y = (desiredHeaderSize.y + desiredContentSize.y) / m_zoom;
}

void EditorNodeGraph::DrawNodeHeader(const NodeInstance& instance, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos, glm::vec2& outDesiredHeaderSize) const
{
	//default is to draw the typename at the top of the node

	const std::string& PrettyNodeName = NodeTypeRegistry::Get().GetTypeInfo(instance.typeGUID).prettyName;

	const float fontSize = GetScaledStyleVar(StyleVar::Node_HeaderFontSize);
	const float textPadding = GetScaledStyleVar(StyleVar::Node_HeaderTextPadding);
	const ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, -1, PrettyNodeName.c_str());
	const float headerHeight = textSize.y + textPadding * 2;

	ImColor headerColor = {};
	if (IsNodeHovered(instance.instanceID))
	{
		headerColor = ImColor(GetStyleColor(StyleColor::Node_HoveredHeaderBgColor));
	}
	else
	{
		headerColor = ImColor(GetStyleColor(StyleColor::Node_HeaderBgColor));
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	glm::vec2 headerMaxPos = { maxScreenPos.x, minScreenPos.y + headerHeight };
	drawList->AddRectFilled(minScreenPos, headerMaxPos, headerColor, 3.f * m_zoom, ImDrawFlags_RoundCornersTop);

	drawList->AddText(ImGui::GetFont(), fontSize, minScreenPos + glm::vec2(textPadding, textPadding), 0xffffffff, PrettyNodeName.c_str());

	outDesiredHeaderSize.x = textSize.x + textPadding * 2;
	outDesiredHeaderSize.y = textSize.y + textPadding * 2;
}

void EditorNodeGraph::DrawNodeContent(const NodeInstance& instance, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax, glm::vec2& outDesiredContentSize) const
{
	const NodeTypeDefinition& typeDef = NodeTypeRegistry::Get().GetTypeDefinition(instance.typeGUID);

	const float contentPadding = GetScaledStyleVar(StyleVar::Node_ContentPadding);
	const float pinRadius = GetScaledStyleVar(StyleVar::Pin_Radius);
	const float pinVPadding = GetScaledStyleVar(StyleVar::Pin_VPadding);
	const float pinHPadding = GetScaledStyleVar(StyleVar::Pin_HPadding);
	const float pinUnconnectedThickness = GetScaledStyleVar(StyleVar::Pin_UnconnectedThickness);
	const float textPadding = GetScaledStyleVar(StyleVar::Pin_TextPadding);
	const float textFontSize = GetScaledStyleVar(StyleVar::Pin_TextFontSize);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 cursorPos = contentAreaScreenMin + ImVec2(contentPadding, contentPadding);
	const float startCursorY = cursorPos.y;
	float widestInputPin = 0;
	for (auto [pinID, pinDef] : typeDef.inputPins)
	{
		//add half diameter of pin to place the center in the correct position
		const ImVec2 pinPos = cursorPos + ImVec2(pinRadius, pinRadius);
		drawList->AddCircle(pinPos, pinRadius, 0xffffffff, 0, pinUnconnectedThickness);

		const float textWidth = ImGui::GetFont()->CalcTextSizeA(textFontSize, FLT_MAX, -1, pinDef.pinName.c_str()).x;
		const ImVec2 textPos = pinPos + ImVec2(pinRadius + textPadding, -textFontSize / 2);
		drawList->AddText(ImGui::GetFont(), textFontSize, textPos, 0xffffffff, pinDef.pinName.c_str());

		const float pinTotalWidth = (textPos.x + textWidth) - cursorPos.x;
		if (widestInputPin < pinTotalWidth)
		{
			widestInputPin = pinTotalWidth;
		}

		cursorPos.y += pinRadius * 2;
		cursorPos.y += pinVPadding;
	}
	const float inputPinsHeight = cursorPos.y - startCursorY;

	cursorPos.x = contentAreaScreenMax.x - contentPadding;
	cursorPos.y = startCursorY;
	float widestOutputPin = 0;
	for (auto [pinID, pinDef] : typeDef.outputPins)
	{
		//add half diameter of pin to place the center in the correct position
		const ImVec2 pinPos = cursorPos + ImVec2(-pinRadius, pinRadius);
		drawList->AddCircle(pinPos, pinRadius, 0xffffffff, 0, pinUnconnectedThickness);

		const float textWidth = ImGui::GetFont()->CalcTextSizeA(textFontSize, FLT_MAX, -1, pinDef.pinName.c_str()).x;
		const ImVec2 textPos = pinPos - ImVec2(pinRadius + textPadding + textWidth, textFontSize / 2);
		drawList->AddText(ImGui::GetFont(), textFontSize, textPos, 0xffffffff, pinDef.pinName.c_str());

		const float pinTotalWidth = cursorPos.x - (textPos.x);
		if (widestOutputPin < pinTotalWidth)
		{
			widestOutputPin = pinTotalWidth;
		}

		cursorPos.y += pinRadius * 2;
		cursorPos.y += pinVPadding;
	}
	const float outputPinsHeight = cursorPos.y - startCursorY;

	const float desiredHeight = glm::max(inputPinsHeight, outputPinsHeight) + contentPadding * 2;
	const float desiredWidth = widestOutputPin + widestInputPin + pinHPadding + contentPadding * 2;
	outDesiredContentSize.x = desiredWidth;
	outDesiredContentSize.y = desiredHeight;
}


void EditorNodeGraph::DrawConnection(const NodeConnection& nodeConnection) const
{}

void EditorNodeGraph::GraphManageNode(const NodeInstance& instance, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos)
{
	ImGui::ItemAdd({ minScreenPos, maxScreenPos }, instance.instanceID);
	if (ImGui::IsItemHovered())
	{
		m_hoveredNode = instance.instanceID;
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

void EditorNodeGraph::UserNodeHandling(NodeGraphBase& NodeGraph)
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
			m_startDraggingNodePositions.insert({ selectedNodeID, NodeGraph.GetNodeInstance(selectedNodeID).position});
		}
	}
	if (m_draggingNodes)
	{
		const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) * (1.f / m_zoom);
		for (const NodeInstanceID& selectedNodeID : m_selectedNodes)
		{
			NodeInstance& instanceInfo = NodeGraph.GetMutableNodeInstance(selectedNodeID);
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

