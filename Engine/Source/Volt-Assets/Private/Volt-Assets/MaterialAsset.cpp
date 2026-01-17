#include "vtassetspch.h"

#include "Volt-Assets/MaterialAsset.h"
#include "Volt-Assets/MaterialCompilerSubSystem.h"

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>

#include <Volt-Renderer/Material/RenderMaterial.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/Texture/Texture2D.h>

#include <RenderCore/Shader/DefaultShaders.h>
#include <RenderCore/Shader/ShaderMap.h>

#include <AssetSystem/AssetFactory.h>
#include <AssetSystem/AssetManager.h>
#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/Archive/ArchiveVersionRegistry.h>

namespace Volt
{
	VT_REGISTER_ASSET_FACTORY(AssetTypes::Material, MaterialAsset);
	VT_REGISTER_CUSTOM_ASSET_METADATA_TYPE(MaterialCustomMetadata, AssetTypes::Material);

	struct MaterialAssetCustomVersion
	{
		enum Type
		{
			BaseVersion = 0,
			AddedMaterialBlendMode = 1,

			VersionPlusOne,
			LatestVersion = VersionPlusOne - 1
		};

		inline static constexpr VoltGUID guid = "{DEBF3641-8728-4993-B65D-20B876E2D6E5}"_guid;
	private:
		MaterialAssetCustomVersion() = default;
	};
	ArchiveVersionRegistrar g_registerMaterialAssetCustomVersion(MaterialAssetCustomVersion::guid, MaterialAssetCustomVersion::LatestVersion, "MaterialAssetCustomVersion");

	MaterialAsset::MaterialAsset()
	{
		m_graph = CreateRef<MaterialGraph>();
		m_renderMaterial = CreateRef<RenderMaterial>(std::string(GetAssetName()), ShaderMap::Get<OpaqueDefaultPixelPS>());
	}

	void MaterialAsset::OnAssetDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state)
	{
		ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(GetAssetHandle());
		const MaterialCustomMetadata& materialCustomMetadata = metadata->GetCustomData<MaterialCustomMetadata>();

		for (const MaterialCustomMetadata::TextureInfo& textureInfo : materialCustomMetadata.textureReferences)
		{
			if (textureInfo.handle == dependencyHandle)
			{
				if (state == AssetChangedState::Loaded)
				{
					AssetReference<Texture2D> textureAsset = g_assetManager->GetAssetImmediately<Texture2D>(dependencyHandle);
					m_renderMaterial->SetTexture(textureInfo.index, RenderTexture(textureAsset->GetImage()));
				}
				else if (state == AssetChangedState::Deleted)
				{
					m_renderMaterial->SetTexture(textureInfo.index, RenderTexture(nullptr));
				}
			
				break;
			}
		}
	}

	void MaterialAsset::Serialize(Archive& archive, ReadOnlyAssetMetadata assetMetadata)
	{
		archive.UseVersion(MaterialAssetCustomVersion::guid);

		m_graph->Serialize(archive);

		if (!archive.IsLoading() || archive.GetVersion(MaterialAssetCustomVersion::guid) >= MaterialAssetCustomVersion::AddedMaterialBlendMode)
		{
			archive << m_materialBlendMode;
		}

		// Set the render materials blend mode.
		m_renderMaterial->SetMaterialBlendMode(m_materialBlendMode);

		if (archive.IsLoading())
		{
			// Setup render material.
			{
				const MaterialCustomMetadata& materialCustomMetadata = assetMetadata->GetCustomData<MaterialCustomMetadata>();

				for (const MaterialCustomMetadata::TextureInfo& textureInfo : materialCustomMetadata.textureReferences)
				{
					RefPtr<RHI::Image> image;

					if (textureInfo.handle != Asset::Null())
					{
						AssetReference<Texture2D> textureAsset;
						if (g_assetManager->TryGetAsset(textureInfo.handle, textureAsset))
						{
							image = textureAsset->GetImage();
						}

						if (textureAsset.IsValid())
						{
							m_referencedTextures.emplace_back(textureAsset);
						}
					}

					if (!image)
					{
						image = Renderer::GetDefaultResources().white1x1;
					}

					m_renderMaterial->SetTexture(textureInfo.index, RenderTexture(image));
				}
			}	

			if (MaterialCompilerSubSystem* compilerSubSystem = SubSystemManager::GetSubSystem<MaterialCompilerSubSystem>(); compilerSubSystem != nullptr)
			{
				// #TODO_AssetSystem: Add asset reference from this function
				RefPtr<MaterialAsset> thisAsset = RefPtr<MaterialAsset>::Attach(this);
				compilerSubSystem->RequestMaterialCompilation(AssetReference<MaterialAsset>(thisAsset));
			}
		}
	}

	void MaterialAsset::OnPreSave(CustomAssetMetadata& customMetadata)
	{
		MaterialCustomMetadata& materialCustomMetadata = customMetadata.GetMutableCustomMetadata<MaterialCustomMetadata>();
		materialCustomMetadata.textureReferences.clear();

		const auto& nodes = m_graph->GetMosaicGraph().GetUnderlyingGraph().GetNodes();
		for (const auto& node : nodes)
		{
			if (node.nodeData->GetGUID() == MosaicNodes::SampleTextureNode::GetStaticGUID())
			{
				Ref<MosaicNodes::SampleTextureNode> sampleTextureNode = std::reinterpret_pointer_cast<MosaicNodes::SampleTextureNode>(node.nodeData);
				const auto textureInfo = sampleTextureNode->GetTextureInfo();
				materialCustomMetadata.textureReferences.emplace_back(textureInfo.textureHandle, textureInfo.textureIndex);
			}
		}
	}

	void MaterialAsset::GatherAssetDependencies(AssetDependencyGatherContext& gatherContext, ReadOnlyAssetMetadata assetMetadata)
	{
		const MaterialCustomMetadata& materialCustomMetadata = assetMetadata->GetCustomData<MaterialCustomMetadata>();

		for (const MaterialCustomMetadata::TextureInfo& textureInfo : materialCustomMetadata.textureReferences)
		{
			gatherContext.AddDependency(textureInfo.handle, AssetDependencyType::Hard);
		}
	}
}
