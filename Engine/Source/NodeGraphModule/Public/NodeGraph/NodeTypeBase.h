#pragma once

#include "NodeGraph/Config.h"
#include "NodeGraph/PinType.h"
#include "NodeGraph/NodeTypeDefinitionBuilder.h"

#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/Containers/VectorVariants.h>

#include <string>

struct PinType_BoolStorage
{
	bool Value = false;
};
DECLARE_PIN_TYPE_STORAGE(Bool, PinType_BoolStorage, "{6386E26A-677A-427B-A672-CCBA9D811DEF}"_guid);

DECLARE_PIN_TYPE(Flow, "{48A9B2EC-1584-42E3-928F-F4EF7D2E6E6F}"_guid);

struct PinType_FloatCustomData
{
	bool isSlider = false;
	float speed = 1.f;
	float minBound = 0;
	float maxBound = 0;
};
struct PinType_FloatStorage
{
	float Value = 0;
};
DECLARE_PIN_TYPE_STORAGE_CUSTOM_DATA(Float, PinType_FloatStorage, PinType_FloatCustomData, "{1AB638AD-561A-4A91-AB9A-2E733BB2AAEC}"_guid);

#define DECLARE_NODE_TYPE(nodeTypeName, prettyName, typeGuid)			\
class nodeTypeName														\
{																		\
public:																	\
	nodeTypeName()														\
	{																	\
		NodeTypeRegistry::Get().RegisterNodeType<nodeTypeName>();		\
	}																	\
	~nodeTypeName()														\
	{																	\
		NodeTypeRegistry::Get().UnregisterNodeType<nodeTypeName>();		\
	}																	\
	static VoltGUID GetStaticTypeGUID() { return typeGuid; }			\
	static String GetStaticTypeName() { return #nodeTypeName; }	\
	static String GetStaticTypePrettyName() { return prettyName; }	\
	static void MakeTypeDefinition(NodeTypeDefinitionBuilder& builder);	\
};

// Must lie in a compilation unit (cpp file)
#define REGISTER_NODE_TYPE(nodeTypeName)	nodeTypeName g_nodeTypeRegistrar_##nodeTypeName
//#define REGISTER_NODE_TYPE_FOR_GRAPH(nodeTypeName) nodeTypeName g_nodeTypeRegistrar_##nodeTypeName


class VTNODE_API NodeTypeRegistry
{
public:
	struct NodeTypeInfo
	{
		VoltGUID typeGUID;
		String typeName;
		String prettyName;
		NodeTypeDefinition typeDefinition;
	};

	static NodeTypeRegistry& Get();

	template<typename T>
	void RegisterNodeType()
	{
		NodeTypeInfo typeInfo;
		typeInfo.typeGUID = T::GetStaticTypeGUID();
		typeInfo.typeName = T::GetStaticTypeName();
		typeInfo.prettyName = T::GetStaticTypePrettyName();

		NodeTypeDefinitionBuilder defBuilder;
		T::MakeTypeDefinition(defBuilder);
		typeInfo.typeDefinition = defBuilder.MoveDefinition();

		m_nodeTypes.insert({ typeInfo.typeGUID, std::move(typeInfo) });
	}
	template<typename T>
	void UnregisterNodeType()
	{
		m_nodeTypes.erase(T::GetStaticTypeGUID());
	}
	const NodeTypeInfo& GetTypeInfo(VoltGUID typeGUID) { return m_nodeTypes[typeGUID]; }
	const NodeTypeDefinition& GetTypeDefinition(VoltGUID typeGUID) {return GetTypeInfo(typeGUID).typeDefinition; }
	bool TypeExists(VoltGUID typeGUID) { return m_nodeTypes.contains(typeGUID); }

private:
	std::unordered_map<VoltGUID, NodeTypeInfo> m_nodeTypes;
};

DECLARE_NODE_TYPE(NothingNode, "Nothing Node", "{E1121BF0-3FCA-4AD6-A3AE-26A9CC85DE7D}"_guid);
