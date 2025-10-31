#pragma once

#include "Volt-MaterialGraph/Config.h"

#include <AssetSystem/Asset_New.h>

#include <Mosaic/MosaicNode.h>

namespace Volt::MosaicNodes
{
	struct TextureInfo
	{
		uint32_t textureIndex;
		AssetHandle textureHandle;
	};

	class SampleTextureNode : public Mosaic::MosaicNode
	{
	public:
		SampleTextureNode(Mosaic::MosaicGraph* ownerGraph);
		~SampleTextureNode() override;

		MOSAIC_NODE_DECLARE_GUID("{DB60F69D-EFC5-4AA4-BF5A-C89D58942D3F}"_guid);

		inline const std::string GetName() const override { return "Sample Texture"; }
		inline const std::string GetCategory() const override { return "Texture"; }
		inline const glm::vec4 GetColor() const override { return 1.f; }

		void Reset() override;
		void SerializeCustom(YAMLStreamWriter& streamWriter) const override;
		void DeserializeCustom(YAMLStreamReader& streamReader) override;

		const Mosaic::ResultInfo Compile(const GraphNode<Ref<class Mosaic::MosaicNode>, Ref<Mosaic::MosaicEdge>>& underlyingNode, uint32_t outputIndex, Mosaic::MosaicShaderWriter& shaderWriter) const override;

		VTMG_API const TextureInfo GetTextureInfo() const;

		VT_INLINE AssetHandle GetTextureHandle() const { return m_textureHandle; }
		VT_INLINE void SetTextureHandle(AssetHandle textureHandle) { m_textureHandle = textureHandle; }

	private:
		uint32_t m_textureIndex = 0;
		AssetHandle m_textureHandle = Asset_New::Null();

		mutable bool m_evaluated = false;
		mutable Mosaic::ResultInfo m_evaluatedResultInfo;
	};
}
