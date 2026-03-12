#pragma once

#include "Volt-Renderer/Config.h"
#include "Volt-Renderer/Material/RenderMaterial.h"

#include <AssetSystem/AssetHandle.h>

namespace Volt
{
	class VTR_API MaterialTable
	{
	public:
		void CreateMaterial(uint32_t index);
		void SetMaterial(Ref<RenderMaterial> material, uint32_t index);
		Ref<RenderMaterial> GetMaterial(uint32_t index) const;

		bool ContainsMaterialIndex(uint32_t index) const;

		inline const bool IsValid() const { return !m_materials.empty(); }
		inline const uint32_t GetSize() const { return static_cast<uint32_t>(m_materials.size()); }

		Vector<Ref<RenderMaterial>>::iterator begin() { return m_materials.begin(); }
		Vector<Ref<RenderMaterial>>::iterator end() { return m_materials.end(); }

		const Vector<Ref<RenderMaterial>>::const_iterator begin() const { return m_materials.cbegin(); }
		const Vector<Ref<RenderMaterial>>::const_iterator end() const { return m_materials.cend(); }
		
	private:
		Vector<Ref<RenderMaterial>> m_materials;
	};
}
