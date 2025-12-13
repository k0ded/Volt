#pragma once

#include "Sandbox/NodeGraph/EditorNodeBase.h"

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
	EditorNodeGraph(std::string_view imGuiID);
	~EditorNodeGraph() = default;

	void Draw();

	template<typename T>
	void RegisterNodeType();

	template<typename T>
	NodeInstanceID SpawnNodeOfType();
	NodeInstanceID SpawnNodeOfType(VoltGUID typeID);

	bool IsNodeSelected(NodeInstanceID instanceID) const;
	bool IsNodeHovered(NodeInstanceID instanceID) const;

protected:
	struct NodeInstanceInfo
	{
		NodeInstanceID instanceID;

		glm::vec2 position;
		Ref<EditorNodeTypeBase> nodeInstance;
	};

	struct NodeConnection
	{
		NodeInstanceID from;
		NodeInstanceID to;
	};
protected:
	virtual void DrawGraph();
	virtual void DrawNode(const NodeInstanceInfo& nodeInstanceInfo);
	virtual void DrawNodeContent(const NodeInstanceInfo& nodeInstanceInfo, const glm::vec2& contentAreaScreenMin, const glm::vec2& contentAreaScreenMax) const;
	//modify the given rect to set the new content area
	virtual void DrawNodeHeader(const NodeInstanceInfo& nodeInstanceInfo, glm::vec2& minScreenPos, glm::vec2& maxScreenPos) const;
	virtual void DrawConnection(const NodeConnection& nodeConnection) const;

	//manages moving, selecting, deleting etc for the node
	virtual void GraphManageNode(const NodeInstanceInfo& nodeInstanceInfo, const glm::vec2& minScreenPos, const glm::vec2& maxScreenPos);

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
	void UserNodeHandling();
	void UserCameraHandling();

	struct NodeTypeInfo
	{
		VoltGUID typeGUID;
		std::string typeName;
		std::function<Ref<EditorNodeTypeBase>()> createInstanceFunc;
	};
	Map<VoltGUID, NodeTypeInfo> m_registeredNodeTypes;

	
	//start at 1 to reserve 0 as null/invalid ID
	NodeInstanceID m_nextNodeInstanceID = 1;
	Map<NodeInstanceID, NodeInstanceInfo> m_nodeInstances;

	std::set<NodeInstanceID> m_selectedNodes;
	NodeInstanceID m_lastUpdateHoveredNode= 0;
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
	static constexpr int32_t GRID_LINE_COLOR = 0x444455ff;
	static constexpr float GRID_LINE_THICKNESS = 2.f;
};


template<typename T>
inline void EditorNodeGraph::RegisterNodeType()
{
	static_assert(std::is_base_of<EditorNodeTypeBase, T>::value, "T must be derived from EditorNodeBase");

	NodeTypeInfo& typeInfo = m_registeredNodeTypes[T::GetStaticTypeGUID()];
	typeInfo.typeGUID = T::GetStaticTypeGUID();
	typeInfo.typeName = T::GetStaticTypeName();
	typeInfo.createInstanceFunc = []() -> Ref<EditorNodeTypeBase> { return CreateRef<T>(); };
}

template<typename T>
inline NodeInstanceID EditorNodeGraph::SpawnNodeOfType()
{
	return SpawnNodeOfType(T::GetStaticTypeGUID());
}
