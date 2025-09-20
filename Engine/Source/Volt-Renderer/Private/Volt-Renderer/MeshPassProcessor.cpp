#include "vrpch.h"

#include "Volt-Renderer/MeshPassProcessor.h"

namespace Volt
{
	MeshPassProcessorRegistry::MeshPassProcessorRegistry()
	{
		m_meshPassProcessorAllocator.Reserve(512 * 1024);
	}
}
