#include "vrpch.h"

#include "Volt-Renderer/Mesh/MeshSource.h"
#include "Volt-Renderer/Mesh/Mesh.h"

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::MeshSource, MeshSource);

	MeshSource::MeshSource()
	{
		m_underlyingMesh = CreateRef<Mesh>();
	}
}
