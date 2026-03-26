#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class NormalStrengthNode : public Mosaic::MosaicNode
	{
	public:
		NormalStrengthNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{EBD5B70A-4329-4431-880D-FC8072D9BFD5}"_guid);

		const String GetName() const override;
		const String GetCategory() const override;
		const glm::vec4 GetColor() const override;

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;
	};
}
