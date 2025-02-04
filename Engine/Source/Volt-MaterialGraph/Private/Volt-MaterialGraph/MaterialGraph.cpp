#include "vtmgpch.h"

#include "Volt-MaterialGraph/MaterialGraph.h"

#include <CoreUtilities/GUIDUtilities.h>

namespace Volt
{
    MaterialGraph::MaterialGraph()
    {
		m_graph = Mosaic::MosaicGraph::CreateDefaultGraph();
		m_materialGUID = GUIDUtilities::GenerateGUID();
    }

	Vector<AssetHandle> MaterialGraph::GetTextureHandles() const
	{
		return Vector<AssetHandle>();
	}
}
