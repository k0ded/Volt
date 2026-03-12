#include "sbpch.h"
#include "Sandbox/NodeGraph/EditorNodeGraph.h"
#include "Sandbox/NodeGraph/PinDrawerRegistry.h"

#include <Volt_imgui_extras/imgui_canvas.h>

#include <imgui.h>
#include <imgui_internal.h>


EditorNodeGraph::EditorNodeGraph(std::string_view imGuiID)
	: m_imGuiID(imGuiID)
	, m_graphScreenAreaTL(0, 0), m_graphScreenAreaBR(0, 0)
	, m_graphScreenSize(0, 0)
	, m_graphVisibleWorldSize(0, 0)
	, m_graphPopupID(GRAPH_POPUP_ID + m_imGuiID)
	, m_nodePopupID(NODE_POPUP_ID + m_imGuiID)

{
	memset(&m_style, 0, sizeof(m_style));
	SetupStyle(m_style);

	m_canvas.SetView(ImVec2(0, 0), ZOOM_LEVELS[m_zoomStep]);


}

void EditorNodeGraph::Draw(NodeGraphBase& nodeGraph)
{
	ImGui::PushID(m_imGuiID.c_str());
	if (ImGui::BeginChild("NodeGraph"))
	{
		UserCameraHandling();
		UserNodeHandling(nodeGraph);

		if (m_canvas.Begin("NodeGraph", ImGui::GetContentRegionAvail()))
		{
			DrawGrid();
			DrawNodes(nodeGraph);
			DrawConnections(nodeGraph);

			m_canvas.End();
		}

		//tool menus
		HandleToolMenuInput(nodeGraph);
		DrawNodeToolMenu(nodeGraph);
		DrawGraphToolMenu(nodeGraph);


		//float scaleMul = 1.f * (1 + ImGui::GetIO().MouseWheel * 0.1f);
		//m_zoom *= scaleMul;

		//ImDrawList* drawList = ImGui::GetWindowDrawList();
		//ImVec2 windowPos = ImGui::GetWindowPos();
		//ImVec2 avail = ImGui::GetContentRegionAvail();
		//ImGui::PushClipRect(windowPos, windowPos + avail, true);
		//auto clipped_clip_rect = drawList->_ClipRectStack.back();
		//ImGui::PopClipRect();

		//auto m_DrawListCommadBufferSize = glm::max(drawList->CmdBuffer.Size - 1, 0);
		//auto m_DrawListStartVertexIndex = drawList->_VtxCurrentIdx + drawList->_CmdHeader.VtxOffset;

		//// Clip rectangle in parent canvas space and move it to local space.
		//clipped_clip_rect.x = (clipped_clip_rect.x - m_cameraPos.x) / m_zoom;
		//clipped_clip_rect.y = (clipped_clip_rect.y - m_cameraPos.y) / m_zoom;
		//clipped_clip_rect.z = (clipped_clip_rect.z - m_cameraPos.x) / m_zoom;
		//clipped_clip_rect.w = (clipped_clip_rect.w - m_cameraPos.y) / m_zoom;
		//ImGui::PushClipRect(ImVec2(clipped_clip_rect.x, clipped_clip_rect.y), ImVec2(clipped_clip_rect.z, clipped_clip_rect.w), false);


		//ImGui::Button("Hello World");
		//// Transform mouse position to local space.
		//auto& io = ImGui::GetIO();
		//io.MousePos = (m_MousePosBackup - m_cameraPos) * m_View.InvScale;
		//io.MousePosPrev = (m_MousePosPrevBackup - m_cameraPos) * m_View.InvScale;
		//for (auto i = 0; i < IM_ARRAYSIZE(m_MouseClickedPosBackup); ++i)
		//	io.MouseClickedPos[i] = (m_MouseClickedPosBackup[i] - m_cameraPos) * m_View.InvScale;

		//m_ViewRect = CalcViewRect(m_View);

		//auto& fringeScale = drawList->_FringeScale;
		//float lastFringeScale = drawList->_FringeScale;
		//fringeScale /= m_zoom;


		////Leave Local Space

		// // Move vertices to screen space.
		//auto vertex = drawList->VtxBuffer.Data + m_DrawListStartVertexIndex;
		//auto vertexEnd = drawList->VtxBuffer.Data + drawList->_VtxCurrentIdx + drawList->_CmdHeader.VtxOffset;

		//// If canvas view is not scaled take a faster path.
		//if (m_zoom != 1.0f)
		//{
		//	while (vertex < vertexEnd)
		//	{
		//		vertex->pos.x = vertex->pos.x * m_zoom + m_cameraPos.x;
		//		vertex->pos.y = vertex->pos.y * m_zoom + m_cameraPos.y;
		//		++vertex;
		//	}

		//	// Move clip rectangles to screen space.
		//	for (int i = m_DrawListCommadBufferSize; i < drawList->CmdBuffer.size(); ++i)
		//	{
		//		auto& command = drawList->CmdBuffer[i];
		//		command.ClipRect.x = command.ClipRect.x * m_zoom + m_cameraPos.x;
		//		command.ClipRect.y = command.ClipRect.y * m_zoom + m_cameraPos.y;
		//		command.ClipRect.z = command.ClipRect.z * m_zoom + m_cameraPos.x;
		//		command.ClipRect.w = command.ClipRect.w * m_zoom + m_cameraPos.y;
		//	}
		//}
		//else
		//{
		//	while (vertex < vertexEnd)
		//	{
		//		vertex->pos.x = vertex->pos.x + m_cameraPos.x;
		//		vertex->pos.y = vertex->pos.y + m_cameraPos.y;
		//		++vertex;
		//	}

		//	// Move clip rectangles to screen space.
		//	for (int i = m_DrawListCommadBufferSize; i < drawList->CmdBuffer.size(); ++i)
		//	{
		//		auto& command = drawList->CmdBuffer[i];
		//		command.ClipRect.x = command.ClipRect.x + m_cameraPos.x;
		//		command.ClipRect.y = command.ClipRect.y + m_cameraPos.y;
		//		command.ClipRect.z = command.ClipRect.z + m_cameraPos.x;
		//		command.ClipRect.w = command.ClipRect.w + m_cameraPos.y;
		//	}
		//}

		//fringeScale = lastFringeScale;

		//// And pop \o/
		//ImGui::PopClipRect();


		/*m_graphScreenSize = ImGui::GetWindowSize();
		m_graphScreenAreaTL = ImGui::GetWindowPos();
		m_graphScreenAreaBR = m_graphScreenAreaTL + m_graphScreenSize;
		m_graphVisibleWorldSize = m_graphScreenSize / m_zoom;

		UserNodeHandling(NodeGraph);
		UserCameraHandling();

		DrawGraph(NodeGraph);*/
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

bool EditorNodeGraph::IsPinHovered(NodeInstanceID instanceID, NodePinID pinID) const
{
	return IsNodeHovered(instanceID) && m_hoveredPin == pinID;
}

void EditorNodeGraph::SetupStyle(Style& style)
{
	//setup defaults
	style.SetVar(StyleVar::Grid_LineThickness, 0.5f);
	style.SetVar(StyleVar::Grid_LineSpacing, 10.f);

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
	style.SetVar(StyleVar::Pin_DefaultItemSize, 50.f);

	style.SetColor(StyleColor::Grid_LineColor, glm::vec4(0.06f, 0.06f, 0.06f, 1.f));

	style.SetColor(StyleColor::Node_BgColor, glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
	style.SetColor(StyleColor::Node_HoveredBgColor, glm::vec4(0.3f, 0.3f, 0.3f, 0.9f));

	style.SetColor(StyleColor::Node_HeaderBgColor, glm::vec4(0.5f, 0.32f, 0.f, 1.f));
	style.SetColor(StyleColor::Node_HoveredHeaderBgColor, glm::vec4(0.6f, 0.42f, 0.1f, 1.f));

	style.SetColor(StyleColor::Node_SelectedOutlineColor, glm::vec4(1.f, 0.647f, 0.f, 1.f));
}

void EditorNodeGraph::DrawNode(NodeInstance& instance)
{
	ImGui::PushID(instance.InstanceID());

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImVec2 nodeSize = GetNodeDimensions(instance.InstanceID()).desiredSize;
	const ImVec2 halfNodeSize = nodeSize / 2.f;

	const glm::vec2 nodePos = instance.position;
	const glm::vec2 nodeMin = nodePos - halfNodeSize;
	const glm::vec2 nodeMax = nodePos + halfNodeSize;

	ImGui::ItemAdd({ nodeMin, nodeMax }, instance.instanceID);
	GraphManageNode(instance);

	if (IsNodeSelected(instance.instanceID))
	{
		drawList->AddRect(nodeMin - ImVec2(1, 1), nodeMax + ImVec2(1, 1), ImColor(GetStyleColor(StyleColor::Node_SelectedOutlineColor)), GetStyleVar(StyleVar::Node_EdgeRounding), 0, 1.f);
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

	drawList->AddRectFilled(nodeMin, nodeMax, nodeColor, GetStyleVar(StyleVar::Node_EdgeRounding));

	glm::vec2 desiredHeaderSize;
	DrawNodeHeader(instance, nodeMin, nodeMax, desiredHeaderSize);

	glm::vec2 desiredContentSize;
	DrawNodeContent(instance, nodeMin + ImVec2(0, desiredHeaderSize.y), nodeMax, desiredContentSize);
	if (glm::length2(desiredContentSize) < 0.0001f)
	{
		desiredContentSize = { 60, 30 };
	}

	ImVec2& desiredSizeRef = GetNodeDimensions(instance.InstanceID()).desiredSize;
	desiredSizeRef.x = glm::max(desiredHeaderSize.x, desiredContentSize.x);
	desiredSizeRef.y = (desiredHeaderSize.y + desiredContentSize.y);

	ImGui::PopID();
}

void EditorNodeGraph::DrawNodeHeader(const NodeInstance& instance, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos, glm::vec2& outDesiredHeaderSize)
{
	const std::string& PrettyNodeName = NodeTypeRegistry::Get().GetTypeInfo(instance.typeGUID).prettyName;

	const float fontSize = GetStyleVar(StyleVar::Node_HeaderFontSize);
	const float textPadding = GetStyleVar(StyleVar::Node_HeaderTextPadding);
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
	drawList->AddRectFilled(minScreenPos, headerMaxPos, headerColor, GetStyleVar(StyleVar::Node_EdgeRounding), ImDrawFlags_RoundCornersTop);

	drawList->AddText(ImGui::GetFont(), fontSize, minScreenPos + glm::vec2(textPadding, textPadding), 0xffffffff, PrettyNodeName.c_str());

	outDesiredHeaderSize.x = textSize.x + textPadding * 2;
	outDesiredHeaderSize.y = textSize.y + textPadding * 2;
}

void EditorNodeGraph::DrawNodeContent(NodeInstance& instance, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax, glm::vec2& outDesiredContentSize)
{
	const NodeTypeDefinition& typeDef = NodeTypeRegistry::Get().GetTypeDefinition(instance.typeGUID);

	const float contentPadding = GetStyleVar(StyleVar::Node_ContentPadding);
	const float pinVPadding = GetStyleVar(StyleVar::Pin_VPadding);
	const float pinHPadding = GetStyleVar(StyleVar::Pin_HPadding);

	//Input pins
	ImGui::SetCursorScreenPos(contentAreaScreenMin + ImVec2(contentPadding, contentPadding));
	const ImVec2 startInputPinsCursorPos = ImGui::GetCursorScreenPos();
	float widestInputPin = 0;
	for (auto& [pinID, pinDef] : typeDef.inputPins)
	{
		const ImVec2 startPinCursorPos = ImGui::GetCursorScreenPos();
		DrawPin(instance, pinDef, instance.GetPinStoragePtr(pinID));
		GraphManagePin(instance, pinID);

		const ImVec2 pinSize = ImGui::GetItemRectSize();
		ImGui::SetCursorScreenPos(startPinCursorPos + ImVec2(0, pinSize.y + pinVPadding));

		if (widestInputPin < pinSize.x)
		{
			widestInputPin = pinSize.x;
		}
	}
	const float inputPinsHeight = ImGui::GetCursorScreenPos().y - startInputPinsCursorPos.y;

	//Output pins
	ImGui::SetCursorScreenPos(ImVec2(contentAreaScreenMax.x, contentAreaScreenMin.y) + ImVec2(-contentPadding, contentPadding));
	const ImVec2 startOutputPinsCursorPos = ImGui::GetCursorScreenPos();
	float widestOutputPin = 0;
	for (auto [pinID, pinDef] : typeDef.outputPins)
	{
		const ImVec2 startPinCursorPos = ImGui::GetCursorScreenPos();
		DrawPin(instance, pinDef, instance.GetPinStoragePtr(pinID));
		GraphManagePin(instance, pinID);

		const ImVec2 pinSize = ImGui::GetItemRectSize();
		ImGui::SetCursorScreenPos(startPinCursorPos + ImVec2(0, pinSize.y + pinVPadding));

		if (widestOutputPin < pinSize.x)
		{
			widestOutputPin = pinSize.x;
		}
	}
	const float outputPinsHeight = ImGui::GetCursorScreenPos().y - startOutputPinsCursorPos.y;

	const float desiredHeight = glm::max(inputPinsHeight, outputPinsHeight) + contentPadding * 2;
	const float desiredWidth = widestOutputPin + widestInputPin + pinHPadding + contentPadding * 2;
	outDesiredContentSize.x = desiredWidth;
	outDesiredContentSize.y = desiredHeight;
}

void EditorNodeGraph::DrawPin(const NodeInstance& instance, const NodePinDefinition& pinDef, void* pinStoragePtr)
{
	VT_ENSURE(pinStoragePtr);
	ImGui::PushID(pinDef.pinID);

	const ImVec2 startCursorPos = ImGui::GetCursorScreenPos();

	const float pinRadius = GetStyleVar(StyleVar::Pin_Radius);
	const ImVec2 pinIconSize = ImVec2(pinRadius * 2, pinRadius * 2);

	const float textPadding = GetStyleVar(StyleVar::Pin_TextPadding);
	const float textFontSize = GetStyleVar(StyleVar::Pin_TextFontSize);
	const char* pinText = pinDef.pinName.c_str();
	const ImVec2 pinTextSize = ImGui::GetFont()->CalcTextSizeA(textFontSize, FLT_MAX, -1, pinText);

	const float pinSizeX = pinIconSize.x + pinTextSize.x + textPadding * 2;
	const float pinSizeY = glm::max(pinIconSize.y, pinTextSize.y);
	const ImVec2 pinSize = ImVec2(pinSizeX, pinSizeY);

	if (IsPinHovered(instance.InstanceID(), pinDef.pinID))
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		const float halfPinVPadding = GetStyleVar(StyleVar::Pin_VPadding);

		ImVec2 pinMin = startCursorPos - ImVec2(0, halfPinVPadding / 2);
		if (pinDef.direction == PinDirection::Output)
		{
			pinMin -= ImVec2(pinSize.x);
		}

		ImVec2 pinMax = pinMin + pinSize + ImVec2(0, halfPinVPadding);
		drawList->AddRectFilled(pinMin, pinMax, 0xffbba5a5, 3.f);
	}

	//Pin Icon
	if (pinDef.direction == PinDirection::Output)
	{
		const ImVec2 screenPos = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(screenPos - ImVec2(pinIconSize.x, 0));
	}
	const ImVec2 pinCenter = ImGui::GetCursorScreenPos() + ImVec2(pinRadius, pinRadius);
	DrawPinIcon(pinCenter, pinRadius);

	//Pin Text
	{
		ImVec2 screenPos = ImGui::GetCursorScreenPos();
		screenPos.y += (pinIconSize.y / 2.f) - (pinTextSize.y / 2.f);
		if (pinDef.direction == PinDirection::Input)
		{
			screenPos.x += pinIconSize.x + textPadding;
		}
		else
		{
			screenPos.x -= pinTextSize.x + textPadding;
		}
		ImGui::SetCursorScreenPos(screenPos);
	}
	DrawPinText(pinDef.pinName.c_str());

	//Custom Drawing
	//if (pinDef.direction == PinDirection::Input)
	//{
	//	const ImVec2 screenPos = ImGui::GetCursorScreenPos();
	//	ImGui::SetCursorScreenPos(screenPos + ImVec2(pinIconSize.x + textPadding, pinIconSize.y / 2));
	//}
	//else
	//{
	//	const ImVec2 screenPos = ImGui::GetCursorScreenPos();
	//	ImGui::SetCursorScreenPos(screenPos - ImVec2(pinTextSize.x + textPadding, -pinIconSize.y / 2));
	//}
	if (PinDrawerRegistry::Get().HasDrawerForType(pinDef.pinTypeGUID))
	{
		//ImGui::BeginGroup();

		//ImGui::PushFont(ImGui::GetFont(), textFontSize);
		//ImGui::GetCurrentWindow()->DC.ItemWidth = defaultItemSize;
		//PinDrawerRegistry::Get().DrawPinType(pinDef.pinTypeGUID, pinStoragePtr, pinDef.pinTypeData.data());
		//ImGui::PopFont();

		//ImGui::EndGroup();
		//pinDrawerDrawnSize = ImGui::GetItemRectSize();
	}


	ImVec2& pinSizeRef = GetNodeDimensions(instance.InstanceID()).pinSizes[pinDef.pinID];
	if (pinSizeRef != pinSize)
	{
		pinSizeRef = pinSize;
	}
	ImGui::GetWindowDrawList()->AddRect(startCursorPos, startCursorPos + pinSize, 0xff0000ff);
	ImGui::ItemAdd(ImRect(startCursorPos, startCursorPos + pinSize), pinDef.pinID);

	ImGui::PopID();

}

void EditorNodeGraph::DrawPinIcon(const ImVec2& center, float radius)
{
	ImGui::GetWindowDrawList()->AddCircle(center, radius, 0xffffffff, 0, GetStyleVar(StyleVar::Pin_UnconnectedThickness));
}

void EditorNodeGraph::DrawPinText(const char* text)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	//style vars
	const float textFontSize = GetStyleVar(StyleVar::Pin_TextFontSize);
	const ImVec2 textPos = ImGui::GetCursorScreenPos();
	drawList->AddText(ImGui::GetFont(), textFontSize, textPos, 0xffffffff, text);
}

void EditorNodeGraph::DrawConnection(const NodeConnection& nodeConnection) const
{}

void EditorNodeGraph::GraphManageNode(const NodeInstance& instance)
{
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenOverlappedByItem))
	{
		m_hoveredNode = instance.instanceID;
		m_anyNodeHoveredThisUpdate = true;
	}
}

void EditorNodeGraph::GraphManagePin(const NodeInstance& parentNodeInstance, NodePinID pinID)
{
	if (!IsNodeHovered(parentNodeInstance.InstanceID()))
	{
		return;
	}
	if (ImGui::IsItemHovered())
	{
		m_hoveredPin = pinID;
		m_anyPinHoveredThisUpdate = true;
	}
}

void EditorNodeGraph::DrawGrid()
{
	const float lineThickness = GetStyleVar(StyleVar::Grid_LineThickness);
	const float lineSpacing = GetStyleVar(StyleVar::Grid_LineSpacing);

	DrawGridLines(lineSpacing, lineThickness);
	DrawGridLines(lineSpacing * 10, lineThickness * 3);
	DrawGridLines(lineSpacing * 100, lineThickness * 6);
}

void EditorNodeGraph::DrawGridLines(float lineSpacing, float lineThickness)
{
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImRect& viewRect = m_canvas.ViewRect();
	const glm::vec4 lineColor = GetStyleColor(StyleColor::Grid_LineColor);

	const float pixelLineThickness = lineThickness * m_canvas.ViewScale();

	if (pixelLineThickness < GetStyleVar(StyleVar::Grid_LineThickness) * 0.35f)
	{
		return;
	}

	if (pixelLineThickness > GetStyleVar(StyleVar::Grid_LineThickness) * 10.f)
	{
		return;
	}

	const float remainderX = viewRect.Min.x - (static_cast<int32_t>(viewRect.Min.x / lineSpacing) * lineSpacing);
	const float startX = viewRect.Min.x - remainderX;
	for (float x = startX; x < viewRect.Max.x; x += lineSpacing)
	{
		drawList->AddLine(ImVec2(x, viewRect.Min.y), ImVec2(x, viewRect.Max.y), ImColor(lineColor), lineThickness);
	}

	const float remainderY = viewRect.Min.y - (static_cast<int32_t>(viewRect.Min.y / lineSpacing) * lineSpacing);
	const float startY = viewRect.Min.y - remainderY;
	for (float y = startY; y < viewRect.Max.y; y += lineSpacing)
	{
		drawList->AddLine(ImVec2(viewRect.Min.x, y), ImVec2(viewRect.Max.x, y), ImColor(lineColor), lineThickness);
	}
}

void EditorNodeGraph::DrawNodes(NodeGraphBase& nodeGraph)
{
	for (auto& [instanceID, instance] : nodeGraph.GetNodeInstancesMap())
	{
		DrawNode(nodeGraph.GetMutableNodeInstance(instanceID));
	}
}

void EditorNodeGraph::DrawConnections(NodeGraphBase& nodeGraph)
{}

void EditorNodeGraph::UserNodeHandling(NodeGraphBase& NodeGraph)
{
	if (!m_anyNodeHoveredThisUpdate)
	{
		m_hoveredNode = 0;
	}
	if (!m_anyPinHoveredThisUpdate)
	{
		m_hoveredPin = 0;
	}
	m_anyNodeHoveredThisUpdate = false;
	m_anyPinHoveredThisUpdate = false;

	if (m_hoveredNode != 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		m_holdingNode = m_hoveredNode;
		m_selectedNodes.clear();
		m_selectedNodes.insert(m_holdingNode);
	}



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
			m_startDraggingNodePositions.insert({ selectedNodeID, NodeGraph.GetNodeInstance(selectedNodeID).position });
		}
	}
	if (m_draggingNodes)
	{
		const ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left) * m_canvas.View().InvScale;
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
			m_startMovingCameraPos = m_canvas.View().Origin;
		}
		m_movingCamera = true;
		m_canvas.SetView(m_startMovingCameraPos + ImGui::GetMouseDragDelta(ImGuiMouseButton_Right), m_canvas.View().Scale);
	}
	else
	{
		m_movingCamera = false;
	}

	const float scrollStepsFloat = ImGui::GetIO().MouseWheel;
	const int scrollSteps = static_cast<int>(scrollStepsFloat);
	if (scrollSteps != 0)
	{
		const int newZoomStep = glm::clamp(m_zoomStep + scrollSteps, 0, ZOOM_LEVELS_COUNT - 1);
		if (newZoomStep != m_zoomStep)
		{
			m_zoomStep = newZoomStep;

			const float newZoom = ZOOM_LEVELS[m_zoomStep];

			const ImVec2 viewOrigin = m_canvas.View().Origin;
			const ImGuiEx::CanvasView newView = ImGuiEx::CanvasView(viewOrigin, newZoom);

			const ImVec2 mouseScreenPos = ImGui::GetMousePos();
			const ImVec2 oldMousePos = m_canvas.ToLocal(mouseScreenPos);
			const ImVec2 newMousePos = m_canvas.ToLocal(mouseScreenPos, newView);

			const ImVec2 offset = (newMousePos - oldMousePos) * newZoom;
			m_canvas.SetView(viewOrigin + offset, newZoom);
		}
	}
}

void EditorNodeGraph::HandleToolMenuInput(NodeGraphBase& nodeGraph)
{
	if (!m_lastFrameMovingCamera)
	{
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup))
		{
			if (m_hoveredNode != 0)
			{
				m_toolMenuPopupNode = m_hoveredNode;
				ImGui::OpenPopup(m_nodePopupID.c_str());
			}
			else
			{
				ImGui::OpenPopup(m_graphPopupID.c_str());
			}
		}
	}
}

void EditorNodeGraph::DrawNodeToolMenu(NodeGraphBase& nodeGraph)
{
	if (ImGui::BeginPopup(m_nodePopupID.c_str()))
	{
		if (ImGui::MenuItem("Delete"))
		{
			nodeGraph.DeleteNode(m_toolMenuPopupNode);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void EditorNodeGraph::DrawGraphToolMenu(NodeGraphBase& nodeGraph)
{
	if (ImGui::BeginPopup(m_graphPopupID.c_str()))
	{
		if (ImGui::MenuItem("New Node"))
		{
			NodeInstanceID spawnedNodeID = nodeGraph.SpawnNodeOfType<NothingNode>();
			nodeGraph.GetMutableNodeInstance(spawnedNodeID).SetPosition(m_canvas.ToLocal(ImGui::GetMousePos()));
		}
		ImGui::EndPopup();
	}
}
