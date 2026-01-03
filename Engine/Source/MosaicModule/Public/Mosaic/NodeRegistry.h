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
		inline void RegisterNode()
		{
			const VoltGUID guid = T::GetStaticGUID();

			Ref<T> tempNode = CreateRef<T>(nullptr);

			VT_ENSURE(!m_registry.contains(guid));

			NodeInfo nodeInfo{};
			nodeInfo.name = tempNode->GetName();
			nodeInfo.category = tempNode->GetCategory();
			nodeInfo.createFunction = [](MosaicGraph* ownerGraph)
			{
				return CreateRef<T>(ownerGraph);
			};

			m_registry[guid] = nodeInfo;
		}

		template<typename T>
		inline void UnregisterNode()
		{
			const VoltGUID guid = T::GetStaticGUID();

			// #Note_Ivar: The registry may already have been destroyed due to
			// DLL ordering.
			if (m_registry.empty())
			{
				return;
			}

			if (VT_CHECK(m_registry.contains(guid)))
			{
				m_registry.erase(guid);
			}
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
#define DECLARE_NODE_TEMPLATE(varName, nodeType) \
	using varName = UNPACK nodeType

// Must lie in a compilation unit (cpp file)
#define REGISTER_NODE(nodeType) \
	class MosaicNodeRegistrar_##nodeType \
	{ \
	public: \
		VT_INLINE MosaicNodeRegistrar_##nodeType() \
		{ \
			::Mosaic::NodeRegistry::Get().RegisterNode<nodeType>(); \
		} \
		VT_INLINE ~MosaicNodeRegistrar_##nodeType() \
		{ \
			::Mosaic::NodeRegistry::Get().UnregisterNode<nodeType>(); \
		} \
	} g_mosaicNodeRegistrar_##nodeType
