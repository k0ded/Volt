#pragma once

#include "Sandbox/NodeGraph/EditorNodeBase.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/Map.h>

#include <cstdint> 

#include <glm/fwd.hpp>

typedef int32_t NodeInstanceID;

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

protected:
	virtual void DrawGraph();

	void DrawGrid();

	glm::vec2 SceenToWorldPos(const glm::vec2& screenPos);
	float SceenToWorldXPos(float screenPos);
	float SceenToWorldYPos(float screenPos);
	glm::vec2 WorldToScreenPos(const glm::vec2& worldPos);
	float WorldToScreenXPos(float worldPos);
	float WorldToScreenYPos(float worldPos);

	glm::vec2 m_cameraPos;
	float m_zoom;

	glm::vec2 m_graphScreenAreaTL;
	glm::vec2 m_graphScreenAreaBR;
	glm::vec2 m_graphScreenSize;
	glm::vec2 m_graphVisibleWorldSize;

private:
	struct NodeTypeInfo
	{
		VoltGUID TypeGUID;
		std::string TypeName;
		std::function<Ref<EditorNodeTypeBase>()> CreateInstanceFunc;
	};
	Map<VoltGUID, NodeTypeInfo> m_registeredNodeTypes;
	Vector<Ref<EditorNodeTypeBase>> m_spawnedNodes;

	bool m_movingCamera;
	glm::vec2 m_startMovingCameraPos;
	std::string m_imGuiID;

	//style settings
	static constexpr int32_t GRID_LINE_COLOR = 0x444455ff;
	static constexpr float GRID_LINE_THICKNESS = 1.f;
};


template<typename T>
inline void EditorNodeGraph::RegisterNodeType()
{
	static_assert(std::is_base_of<EditorNodeTypeBase, T>::value, "T must be derived from EditorNodeBase");

	NodeTypeInfo& typeInfo = m_registeredNodeTypes[typeInfo.TypeGUID];
	typeInfo.TypeGUID = T::GetStaticTypeGUID();
	typeInfo.TypeName = T::GetStaticTypeName();
	typeInfo.CreateInstanceFunc = []() -> Ref<EditorNodeTypeBase> { return CreateRef<T>(); };
}

template<typename T>
inline NodeInstanceID EditorNodeGraph::SpawnNodeOfType()
{
	return SpawnNodeOfType(T::GetStaticTypeGUID());
}
