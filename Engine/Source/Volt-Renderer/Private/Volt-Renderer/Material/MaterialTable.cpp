#include "vrpch.h"
#include "Volt-Renderer/Material/MaterialTable.h"

#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/GlobalShaderMap.h>

namespace Volt
{
	void MaterialTable::SetMaterial(Ref<RenderMaterial> material, uint32_t index)
	{
		if (static_cast<size_t>(index) >= m_materials.size())
		{
			m_materials.resize(index + 1);
		}

		m_materials[index] = material;
	}

	void MaterialTable::CreateMaterial(uint32_t index)
	{
		if (static_cast<size_t>(index) >= m_materials.size())
		{
			m_materials.resize(index + 1);
		}

		if (!m_materials.at(index))
		{
			m_materials[index] = CreateRef<RenderMaterial>("Null");
		}
	}

	Ref<RenderMaterial> MaterialTable::GetMaterial(uint32_t index) const
	{
		VT_ASSERT_MSG(static_cast<size_t>(index) < m_materials.size(), "Trying to access invalid material!");
		return m_materials.at(index);
	}

	bool MaterialTable::ContainsMaterialIndex(uint32_t index) const
	{
		return static_cast<size_t>(index) < m_materials.size();
	}
}
