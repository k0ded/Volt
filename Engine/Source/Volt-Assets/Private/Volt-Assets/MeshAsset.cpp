#include "vtassetspch.h"

#include "Volt-Assets/MeshAsset.h"
#include "Volt-Assets/MaterialAsset.h"

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager_New.h>
#include <AssetSystem/AssetLocks.h>

#include <CoreUtilities/Profiling/Profiling.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Mesh, MeshAsset);

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
						ScopedAssetReferenceLock assetLock{ materialAsset };
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
					m_materials[i] = Asset_New::Null();
					m_mesh->SetMaterial(Renderer::GetDefaultResources().defaultMaterial, i);
					break;
				}
			}
		}
	}

	void MeshAsset::Initialize(const MeshInitializer& meshInitializer, const Vector<AssetReference<MaterialAsset>>& materials)
	{
		VT_PROFILE_FUNCTION();
		VT_ENSURE_MSG(!m_isInitialized, "A mesh should not be initialized more than once!");

		m_materials.resize(materials.size());
		for (size_t i = 0; i < m_materials.size(); ++i)
		{
			ScopedAssetReferenceLock lock{ materials[i] };
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

			Ref<RenderMaterial> renderMaterial;

			AssetReference<MaterialAsset> materialAsset;
			if (g_assetManager->TryGetAssetIfLoaded(materialHandle, materialAsset))
			{
				ScopedAssetReferenceLock assetLock{ materialAsset };
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
