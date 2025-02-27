#pragma once

#include "Volt-MaterialGraph/Config.h"

#include <AssetSystem/AssetHandle.h>

#include <Mosaic/MosaicGraph.h>

namespace Volt
{
	class VTMG_API MaterialGraph
	{
	public:
		MaterialGraph();

		VT_NODISCARD VT_INLINE Mosaic::MosaicGraph& GetMosaicGraph() { return *m_graph; }
		VT_NODISCARD VT_INLINE const Mosaic::MosaicGraph& GetMosaicGraph() const { return *m_graph; }
		VT_NODISCARD VT_INLINE const VoltGUID& GetMaterialGUID() const { return m_materialGUID; }

		Vector<AssetHandle> GetTextureHandles() const;

	private:
		friend class MaterialSerializer;

		Scope<Mosaic::MosaicGraph> m_graph;
		VoltGUID m_materialGUID = VoltGUID::Null();
	};
}
