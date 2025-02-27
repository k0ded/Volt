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
			for (const auto& [index, materialHandle] : m_materials)
			{
				if (materialHandle == dependencyHandle)
				{
					if (AssetManager::IsLoaded(materialHandle))
					{
						Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
						m_mesh->SetMaterial(materialAsset->GetRenderMaterial(), index);
					}
					else
					{
						m_mesh->SetMaterial(Renderer::GetDefaultResources().defaultMaterial, index);
					}
					break;
				}
			}
		}
		else if (state == AssetChangedState::Removed)
		{
			for (auto& [index, materialHandle] : m_materials)
			{
				if (materialHandle == dependencyHandle)
				{
					materialHandle = Asset::Null();

					m_mesh->SetMaterial(Renderer::GetDefaultResources().defaultMaterial, index);
					break;
				}
			}
		}
	}

	void MeshAsset::FinalizeDeserialization()
	{
		for (const auto& [index, materialHandle] : m_materials)
		{
			if (AssetManager::IsLoaded(materialHandle))
			{
				Ref<MaterialAsset> materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
				m_mesh->SetMaterial(materialAsset->GetRenderMaterial(), index);
			}
			else
			{
				m_mesh->SetMaterial(Renderer::GetDefaultResources().defaultMaterial, index);
			}
		}

		m_mesh->SetName(assetName);
		m_mesh->Construct();
	}
}
