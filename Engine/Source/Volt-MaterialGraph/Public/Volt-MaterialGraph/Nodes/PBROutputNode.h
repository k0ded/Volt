#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class PBROutputNode : public Mosaic::MosaicNode
	{
	public:
		PBROutputNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{343B2C0A-C4E3-41BB-8629-F9939795AC76}"_guid)

		inline const String GetName() const override { return "PBR Output"; }
		inline const String GetCategory() const override { return "Output"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;

	private:
	};
}
