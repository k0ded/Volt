#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class DeriveNormalZNode : public Mosaic::MosaicNode
	{
	public:
		DeriveNormalZNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{2FB746AE-7B1E-48D3-B68D-D970DDED7A4A}"_guid);

		const String GetName() const override;
		const String GetCategory() const override;
		const glm::vec4 GetColor() const override;

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;
	};
}
