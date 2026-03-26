#pragma once

#include <NodeGraph/NodeGraphBase.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Map.h>

#include <cstdint> 
#include <set>

#include <Volt_imgui_extras/imgui_canvas.h>

#include <glm/fwd.hpp>

struct ImRect;

typedef uint32_t NodeInstanceID;

class EditorNodeGraph
{
public:
	enum class StyleVar : uint8_t;
	enum class StyleColor : uint8_t;
public:
	EditorNodeGraph(StringView imGuiID);
	~EditorNodeGraph() = default;

	void Draw(NodeGraphBase& NodeGraph);

	bool IsNodeSelected(NodeInstanceID instanceID) const;
	bool IsNodeHovered(NodeInstanceID instanceID) const;

	bool IsPinHovered(NodeInstanceID instanceID, NodePinID pinID) const;

	float GetStyleVar(StyleVar var) const { return m_style.styleVars[static_cast<uint8_t>(var)];}
	glm::vec4 GetStyleColor(StyleColor color) const { return m_style.styleColors[static_cast<uint8_t>(color)]; }

public:
	enum class StyleVar : uint8_t
	{
		Grid_LineThickness, // thickness of the grid lines
		Grid_LineSpacing, // spacing between grid lines

		Node_HeaderFontSize, // font size of the header text
		Node_HeaderTextPadding, // padding from the outer edges for the text of the header 

		Node_EdgeRounding, // rounding of the outer corners for nodes
		Node_ContentPadding, // padding for the content of a node from all outer sides of a node

		Pin_Radius, // half size of pins on a node
		Pin_VPadding, // vertical padding between pins
		Pin_HPadding, // horizontal padding between input and output pins
		Pin_UnconnectedThickness, // thickness of the pin outline when not connected
		Pin_TextPadding, // padding between a pin and the display name of the pin
		Pin_TextFontSize, // font size of the display name text for a pin
		Pin_DefaultItemSize, // default size of items drawn by pin drawers

		COUNT
	};
	enum class StyleColor : uint8_t
	{
		Grid_LineColor, // color of the grid lines

		Node_BgColor, // background color of nodes
		Node_HoveredBgColor, // background color of nodes when hovered 

		Node_HeaderBgColor, // background color of the header on nodes
		Node_HoveredHeaderBgColor, // background color of the header on nodes

		Node_SelectedOutlineColor, // color of the outline for selected nodes

		COUNT
	};

	struct Style
	{
		float styleVars[static_cast<uint8_t>(StyleVar::COUNT)];
		glm::vec4 styleColors[static_cast<uint8_t>(StyleColor::COUNT)];

		void SetVar(StyleVar var, float value) { styleVars[static_cast<uint8_t>(var)] = value; }
		void SetColor(StyleColor color, glm::vec4 value){ styleColors[static_cast<uint8_t>(color)] = value; }
	};
protected:
	virtual void SetupStyle(Style& style);

	struct NodeDimensions
	{
		ImVec2 desiredSize = { 60,40 };
		Map<NodePinID, ImVec2> pinSizes;
		ImVec2 inputPinsAreaSize = { 1,1 };
		ImVec2 outputPinsAreaSize = { 1,1 };
	};
	NodeDimensions& GetNodeDimensions(NodeInstanceID instanceID)
	{
		return m_nodeInstanceDimensionsMap[instanceID];
	}
	Map<NodeInstanceID, NodeDimensions> m_nodeInstanceDimensionsMap;

	virtual void DrawNode(NodeInstance& instance);
	//modify the given rect to set the new content area
	virtual void DrawNodeHeader(const NodeInstance& instance, const glm::vec2& nodeMin, const glm::vec2& nodeMax, glm::vec2& outDesiredHeaderSize);
	virtual void DrawNodeContent(NodeInstance& instance, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax, glm::vec2& outDesiredContentSize);
	virtual void DrawPin(const NodeInstance& instance, const NodePinDefinition& pinDef, void* pinStoragePtr);
	virtual void DrawPinIcon(const ImVec2& center, float radius);
	//text y is centered on current CursorScreenPos
	virtual void DrawPinText(const char* text);
	virtual void DrawConnection(const NodeConnection& nodeConnection) const;

	//manages moving, selecting, deleting etc for the node
	virtual void GraphManageNode(const NodeInstance& instance);
	virtual void GraphManagePin(const NodeInstance& parentNodeInstance, NodePinID pinID);

	virtual void DrawGrid();
	virtual void DrawGridLines(float lineSpacing, float lineThickness);

	virtual void DrawNodes(NodeGraphBase& nodeGraph);
	virtual void DrawConnections(NodeGraphBase& nodeGraph);

	static constexpr float ZOOM_LEVELS[] =
	{
		0.1f, 0.15f, 0.20f, 0.25f, 0.33f, 0.5f, 0.75f, 1.0f,
		1.25f, 1.50f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f
	};
	static constexpr int ZOOM_LEVELS_COUNT = sizeof(ZOOM_LEVELS) / sizeof(float);
	int m_zoomStep = 7; // zoom step 7 is 1.0f

	glm::vec2 m_graphScreenAreaTL;
	glm::vec2 m_graphScreenAreaBR;
	glm::vec2 m_graphScreenSize;
	glm::vec2 m_graphVisibleWorldSize;

private:
	void UserNodeHandling(NodeGraphBase& nodeGraph);
	void UserCameraHandling();
	void HandleToolMenuInput(NodeGraphBase& nodeGraph);
	void DrawNodeToolMenu(NodeGraphBase& nodeGraph);
	void DrawGraphToolMenu(NodeGraphBase& nodeGraph);

	std::set<NodeInstanceID> m_selectedNodes;
	NodeInstanceID m_lastUpdateHoveredNode = 0;
	NodeInstanceID m_hoveredNode = 0;
	NodeInstanceID m_holdingNode = 0;
	NodeInstanceID m_toolMenuPopupNode = 0;
	bool m_anyNodeHoveredThisUpdate = false;

	NodeInstanceID m_hoveredPin = 0;
	bool m_anyPinHoveredThisUpdate = false;

	bool m_draggingNodes = false;
	Map<NodeInstanceID, glm::vec2> m_startDraggingNodePositions;

	bool m_lastFrameMovingCamera;
	bool m_movingCamera;
	glm::vec2 m_startMovingCameraPos;
	String m_imGuiID;

	ImGuiEx::Canvas m_canvas;

	//style settings
	Style m_style;

	static constexpr int32_t GRID_LINE_COLOR = 0x444455ff;
	static constexpr float GRID_LINE_THICKNESS = 2.f;

	static constexpr const char* GRAPH_POPUP_ID = "GRAPH_TOOLMENU";
	const String m_graphPopupID;

	static constexpr const char* NODE_POPUP_ID = "NODE_TOOLMENU";
	const String m_nodePopupID;
};
