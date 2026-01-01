#pragma once

#include "Mosaic/Config.h"

#include <CoreUtilities/Core.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/VoltAssert.h>

#include <unordered_map>
#include <functional>

namespace Mosaic
{
	class MosaicNode;
	class MosaicGraph;

	struct NodeInfo
	{
		std::function<Ref<MosaicNode>(MosaicGraph* ownerGraph)> createFunction;
		std::string name;
		std::string category;
	};

	class VTMOSAIC_API NodeRegistry
	{
	public:
		template<typename T>
		inline bool RegisterNode()
		{
			// Instantiate node to get GUID
			Ref<T> tempNode = CreateRef<T>(nullptr);
			const VoltGUID guid = tempNode->GetGUID();

			if (m_registry.contains(guid))
			{	
				return false;
			}

			NodeInfo nodeInfo{};
			nodeInfo.name = tempNode->GetName();
			nodeInfo.category = tempNode->GetCategory();
			nodeInfo.createFunction = [](MosaicGraph* ownerGraph)
			{
				return CreateRef<T>(ownerGraph);
			};

			m_registry[guid] = nodeInfo;
			return true;
		}

		inline Ref<MosaicNode> CreateNode(const VoltGUID guid, MosaicGraph* ownerGraph)
		{
			VT_ENSURE(m_registry.contains(guid));
			return m_registry.at(guid).createFunction(ownerGraph);
		}

		inline const auto& GetRegistry() { return m_registry; }
		inline const auto& GetNodeInfo(const VoltGUID guid) { return m_registry.at(guid); }

		static NodeRegistry& Get();

	private:
		std::unordered_map<VoltGUID, NodeInfo> m_registry;
	};
}

#define UNPACK(...) __VA_ARGS__
#define REGISTER_NODE(nodeType) inline static bool nodeType ## _node_registered = ::Mosaic::NodeRegistry::Get().RegisterNode<nodeType>()

#define DECLARE_NODE_TEMPLATE(varName, nodeType) \
	using varName = UNPACK nodeType

#define REGISTER_NODE_TEMPLATE(nodeType) \
	inline static bool nodeType ## _node_registered = ::Mosaic::NodeRegistry::Get().RegisterNode<nodeType>()
