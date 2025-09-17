#include "vtassetspch.h"

#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Mesh, MeshAsset);

	MeshAsset::MeshAsset()
	{
		m_mesh = CreateRef<Mesh>();
	}

	void MeshAsset::OnDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state)
	{
		if (state == AssetChangedState::Updated)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
			{
				const AssetHandle materialHandle = m_materials.at(i);
				if (materialHandle == dependencyHandle)
				{
					Ref<RenderMaterial> renderMaterial;

					if (AssetManager::IsLoaded(materialHandle))
					{
						Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
						renderMaterial = materialAsset->GetRenderMaterial();
					}
					else
					{
						renderMaterial = Renderer::GetDefaultResources().defaultMaterial;
					}
					m_mesh->SetMaterial(renderMaterial, i);
					break;
				}
			}
		}
		else if (state == AssetChangedState::Removed)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
			{
				const AssetHandle materialHandle = m_materials.at(i);
				if (materialHandle == dependencyHandle)
				{
					m_materials[i] = Asset::Null();
					m_mesh->SetMaterial(Renderer::GetDefaultResources().defaultMaterial, i);
					break;
				}
			}
		}
	}

	void MeshAsset::Initialize(const MeshInitializer& meshInitializer, const Vector<Ref<MaterialAsset>>& materials)
	{
		VT_PROFILE_FUNCTION();

		m_materials.resize(materials.size());
		for (size_t i = 0; i < m_materials.size(); ++i)
		{
			m_materials[i] = materials[i]->handle;
		}

		m_mesh->SetName(assetName);
		m_mesh->Initialize(meshInitializer);
	}

	void MeshAsset::Initialize(MeshInitializer& meshInitializer, const Vector<AssetHandle>& materials)
	{
		m_materials.resize(materials.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
		{
			const AssetHandle materialHandle = materials.at(i);

			Ref<RenderMaterial> renderMaterial;

			if (AssetManager::IsLoaded(materialHandle))
			{
				Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
				renderMaterial = materialAsset->GetRenderMaterial();
			}
			else
			{
				renderMaterial = Renderer::GetDefaultResources().defaultMaterial;
			}
			meshInitializer.AddMaterial(renderMaterial, i);
		}

		m_mesh->SetName(assetName);
		m_mesh->Initialize(meshInitializer);
	}
}
