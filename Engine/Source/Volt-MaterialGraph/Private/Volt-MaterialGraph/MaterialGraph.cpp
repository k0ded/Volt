#include "vtmgpch.h"

#include "Volt-MaterialGraph/MaterialGraph.h"

#include <Volt-Platforms/Platform.h>

namespace Volt
{
    MaterialGraph::MaterialGraph()
    {
		m_graph = Mosaic::MosaicGraph::CreateDefaultGraph();
		m_materialGUID = PlatformMisc::GenerateGUID();
    }

	Vector<AssetHandle> MaterialGraph::GetTextureHandles() const
	{
		return Vector<AssetHandle>();
	}
}
