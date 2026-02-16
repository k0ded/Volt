#pragma once

#include <NodeGraph/NodeGraphBase.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Map.h>

#include <cstdint> 
#include <set>

#include <glm/fwd.hpp>

typedef uint32_t NodeInstanceID;

class EditorNodeGraph
{
public:
	enum class StyleVar : uint8_t;
	enum class StyleColor : uint8_t;
public:
	EditorNodeGraph(std::string_view imGuiID);
	~EditorNodeGraph() = default;

	void Draw(NodeGraphBase& NodeGraph);

	bool IsNodeSelected(NodeInstanceID instanceID) const;
	bool IsNodeHovered(NodeInstanceID instanceID) const;

	float GetStyleVar(StyleVar var) const { return m_style.styleVars[static_cast<uint8_t>(var)];}
	float GetScaledStyleVar(StyleVar var) const { return m_style.styleVars[static_cast<uint8_t>(var)] * m_zoom; }
	glm::vec4 GetStyleColor(StyleColor color) const { return m_style.styleColors[static_cast<uint8_t>(color)]; }

public:
	enum class StyleVar : uint8_t
	{
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

		COUNT
	};
	enum class StyleColor : uint8_t
	{
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

	virtual void DrawGraph(NodeGraphBase& NodeGraph);
	virtual void DrawNode(NodeInstance& instance);
	//modify the given rect to set the new content area
	virtual void DrawNodeHeader(const NodeInstance& instance, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos, glm::vec2& outDesiredHeaderSize) const;
	virtual void DrawNodeContent(const NodeInstance& instance, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax, glm::vec2& outDesiredContentSize) const;
	virtual void DrawConnection(const NodeConnection& nodeConnection) const;

	//manages moving, selecting, deleting etc for the node
	virtual void GraphManageNode(const NodeInstance& instance, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos);

	virtual void DrawGrid();

	glm::vec2 ScreenToWorldPos(const glm::vec2& screenPos) const;
	float ScreenToWorldXPos(float screenPos) const;
	float ScreenToWorldYPos(float screenPos) const;
	glm::vec2 WorldToScreenPos(const glm::vec2& worldPos) const;
	float WorldToScreenXPos(float worldPos) const;
	float WorldToScreenYPos(float worldPos) const;

	glm::vec2 m_cameraPos;
	float m_zoom;

	glm::vec2 m_graphScreenAreaTL;
	glm::vec2 m_graphScreenAreaBR;
	glm::vec2 m_graphScreenSize;
	glm::vec2 m_graphVisibleWorldSize;

private:
	void UserNodeHandling(NodeGraphBase& NodeGraph);
	void UserCameraHandling();

	std::set<NodeInstanceID> m_selectedNodes;
	NodeInstanceID m_lastUpdateHoveredNode = 0;
	NodeInstanceID m_hoveredNode = 0;
	NodeInstanceID m_holdingNode = 0;
	NodeInstanceID m_toolMenuPopupNode = 0;

	Vector<NodeConnection> m_nodeConnections;

	bool m_draggingNodes = false;
	Map<NodeInstanceID, glm::vec2> m_startDraggingNodePositions;

	bool m_lastFrameMovingCamera;
	bool m_movingCamera;
	glm::vec2 m_startMovingCameraPos;
	std::string m_imGuiID;


	//style settings
	Style m_style;

	static constexpr int32_t GRID_LINE_COLOR = 0x444455ff;
	static constexpr float GRID_LINE_THICKNESS = 2.f;
};
