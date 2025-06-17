#pragma once

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	class UVNode : public Mosaic::MosaicNode
	{
	public:
		UVNode(Mosaic::MosaicGraph* ownerGraph);
		~UVNode() override;

		MOSAIC_NODE_DECLARE_GUID("{565B7926-3604-4417-AE03-4798103A978A}"_guid);

		inline const std::string GetName() const override { return "UV"; }
		inline const std::string GetCategory() const override { return "Texture"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		void Reset() override;
		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;

	private:
		mutable bool m_evaluated = false;
		mutable std::string m_evaluatedVariableName;
	};
}
