#include "vtassetspch.h"

#include "Volt-Assets/MaterialAsset.h"

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-Renderer/RenderMaterial.h>
#include <Volt-Renderer/Texture/Texture2D.h>

#include <AssetSystem/AssetManager.h>

namespace Volt
{
	MaterialAsset::MaterialAsset()
	{
		m_graph = CreateRef<MaterialGraph>();
		m_renderMaterial = CreateRef<RenderMaterial>(assetName);
	}

	void MaterialAsset::OnDependencyChanged(AssetHandle dependencyHandle, AssetChangedState state)
	{
		const auto textureHandles = m_graph->GetTextureHandles();
		for (uint32_t index = 0; const auto& textureHandle : textureHandles)
		{
			if (textureHandle == dependencyHandle)
			{
				if (state == AssetChangedState::Updated)
				{
					Ref<Texture2D> textureAsset = AssetManager::GetAsset<Texture2D>(dependencyHandle);
					m_renderMaterial->SetTexture(index, RenderTexture(textureAsset->GetResourceHandle()));
				}
				else if (state == AssetChangedState::Removed)
				{
					m_renderMaterial->SetTexture(index, RenderTexture(Resource::Invalid));
				}

				break;
			}

			index++;
		}
	}
}
