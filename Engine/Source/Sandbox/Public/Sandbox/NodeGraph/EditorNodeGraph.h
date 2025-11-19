#pragma once

#include "Sandbox/NodeGraph/EditorNodeBase.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Core.h>

#include <cstdint> 

typedef int32_t NodeInstanceID;

class EditorNodeGraph
{
public:
	EditorNodeGraph() = default;
	~EditorNodeGraph() = default;

	template<typename T>
	void RegisterNodeType();

	template<typename T>
	NodeInstanceID SpawnNodeOfType();
	NodeInstanceID SpawnNodeOfType(VoltGUID typeID);

private:
	struct NodeTypeInfo
	{
		VoltGUID TypeGUID;
		std::string TypeName;
		std::function<Ref<EditorNodeTypeBase>()> CreateInstanceFunc;
	};
	Map<VoltGUID, NodeTypeInfo> m_registeredNodeTypes;

	Vector<Ref<EditorNodeTypeBase>> m_spawnedNodes;
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
