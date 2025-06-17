#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class AddNode : public Mosaic::MosaicNode
	{
	public:
		AddNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{08434406-3093-4AA2-B3B8-2B39AA0EE744}"_guid);

		inline const std::string GetName() const override { return "Add"; }
		inline const std::string GetCategory() const override { return "Math"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;

	private:
	};

	class MultiplyNode : public Mosaic::MosaicNode
	{
	public:
		MultiplyNode(Mosaic::MosaicGraph* ownerGraph);

		MOSAIC_NODE_DECLARE_GUID("{DF8DCC43-BD0C-42A9-A63D-AE8C1C496A8E}"_guid);

		inline const std::string GetName() const override { return "Multiply"; }
		inline const std::string GetCategory() const override { return "Math"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;
	};
}
