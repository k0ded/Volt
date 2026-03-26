#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class RemapNormalNode : public Mosaic::MosaicNode
	{
	public:
		RemapNormalNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{8C5F56B0-5078-48B4-88E0-6D962506DBF0}"_guid);

		const String GetName() const override;
		const String GetCategory() const override;
		const glm::vec4 GetColor() const override;

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;
	};
}
