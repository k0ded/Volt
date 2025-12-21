#include "vtassetspch.h"

#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Mesh, MeshAsset);
	REGISTER_CUSTOM_ASSET_METADATA_TYPE(MeshCustomMetadata, AssetTypes::Mesh);

	MeshAsset::MeshAsset()
	{
		m_mesh = CreateRef<Mesh>();
	}

	void MeshAsset::OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state)
	{
		if (state == AssetChangedState::Loaded)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
			{
				const AssetHandle materialHandle = m_materials.at(i);
				if (materialHandle == dependencyHandle)
				{
					Ref<RenderMaterial> renderMaterial;

					AssetReference<MaterialAsset> materialAsset;
					if (g_assetManager->TryGetAssetIfLoaded(materialHandle, materialAsset))
					{
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
		else if (state == AssetChangedState::Deleted)
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

	void MeshAsset::OnPreSave(CustomAssetMetadata& customMetadata)
	{
		MeshCustomMetadata& meshCustomMetadata = customMetadata.GetMutableCustomMetadata<MeshCustomMetadata>();
		meshCustomMetadata.materials = m_materials;
	}

	void MeshAsset::Serialize(Archive& archive)
	{
		archive << m_materials;
	
		// Setup materials
		if (archive.IsLoading())
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
			{
				const AssetHandle materialHandle = m_materials.at(i);

				Ref<RenderMaterial> renderMaterial;

				AssetReference<MaterialAsset> materialAsset;
				if (g_assetManager->TryGetAsset(materialHandle, materialAsset))
				{
					renderMaterial = materialAsset->GetRenderMaterial();
				}
				else
				{
					renderMaterial = Renderer::GetDefaultResources().defaultMaterial;
				}

				if (materialAsset.IsValid())
				{
					m_materialReferences.emplace_back(materialAsset);
				}

				m_mesh->SetMaterial(renderMaterial, i);
			}
		}
		
		// Mesh serialize will initialize the mesh.
		m_mesh->Serialize(archive);
	}

	void MeshAsset::Initialize(const MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!m_isInitialized, "A mesh should not be initialized more than once!");

		m_materials.resize(materials.size());
		for (size_t i = 0; i < m_materials.size(); ++i)
		{
			m_materials[i] = materials[i]->GetAssetHandle();
		}

		m_mesh->SetName(std::string(GetAssetName()));
		m_mesh->Initialize(meshInitializer);

		m_isInitialized = true;
	}

	void MeshAsset::Initialize(MeshInitializer& meshInitializer, const Vector<AssetHandle>& materials)
	{
		VT_ENSURE_MSG(!m_isInitialized, "A mesh should not be initialized more than once!");

		m_materials.resize(materials.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_materials.size()); ++i)
		{
			const AssetHandle materialHandle = materials.at(i);
			m_materials[i] = materialHandle;

			Ref<RenderMaterial> renderMaterial;

			AssetReference<MaterialAsset> materialAsset;
			if (g_assetManager->TryGetAssetIfLoaded(materialHandle, materialAsset))
			{
				renderMaterial = materialAsset->GetRenderMaterial();
			}
			else
			{
				renderMaterial = Renderer::GetDefaultResources().defaultMaterial;
			}
			meshInitializer.AddMaterial(renderMaterial, i);
		}

		m_mesh->SetName(std::string(GetAssetName()));
		m_mesh->Initialize(meshInitializer);

		m_isInitialized = true;
	}
}
