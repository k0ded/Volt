#include "sbpch.h"
#include "AssetRegistryPanel.h"

#include "Sandbox/Utility/EditorResources.h"

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Renderer/Texture/Texture2D.h>

#include <AssetSystem/AssetManager.h>

#include <JobSystem/JobSystem.h>
#include <JobSystem/TaskGraph.h>
#include <CoreUtilities/StringUtility.h>

AssetRegistryPanel::AssetRegistryPanel()
	: EditorWindow("Asset Registry")
{
	m_searchString = "";
	m_wantsSwap = false;
	m_assetHandles = &m_intermediateAssetHandles_1;
}

AssetRegistryPanel::~AssetRegistryPanel()
{}

void AssetRegistryPanel::UpdateMainContent()
{
	VT_PROFILE_FUNCTION();

	if (!g_assetManager->GetMetadataLoadingCounter()->IsCompleted())
	{
		ImGui::Text("Asset Manager Loading Metadatas...");
		return;
	}

	SwapOpportunity();
	if (m_updateQueued)
	{
		if (DispatchUpdateMetadata())
		{
			m_updateQueued = false;
		}
	}

	if (ImGui::InputTextWithHintString("##AssetRegistrySearch", "Search Asset Registry...", &m_searchString))
	{
		OnSearchChanged();
	}

	constexpr uint32_t NUM_COLUMNS = 4;
	constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp |
		ImGuiTableFlags_HighlightHoveredColumn | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY;
	if (ImGui::BeginTable("AssetRegistryTable", NUM_COLUMNS, flags, ImVec2(ImGui::GetContentRegionAvail().x, 0.f)))
	{
		ImGui::TableSetupColumn("Handle");
		ImGui::TableSetupColumn("Type");
		ImGui::TableSetupColumn("Path");
		ImGui::TableSetupColumn("Load State");

		//draw headers
		{
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
			int32_t columnIndex = 0;
			for (int32_t column = 0; column < NUM_COLUMNS; column++)
			{
				ImGui::TableSetColumnIndex(columnIndex);
				columnIndex++;

				const char* column_name = ImGui::TableGetColumnName(); // Retrieve name passed to TableSetupColumn()
				ImGui::TableHeader(column_name);
			}
		}

		// Sort our data if sort specs have been changed
		if (ImGuiTableSortSpecs* sort_specs = ImGui::TableGetSortSpecs())
		{
			if (sort_specs->SpecsDirty)
			{
				//Sort(sort_specs); // todo: implement
				sort_specs->SpecsDirty = false;
			}
		}

		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(m_assetHandles->size()));
		while (clipper.Step())
		{
			for (int n = clipper.DisplayStart; n < clipper.DisplayEnd; n++)
			{
				Volt::AssetHandle handle = (*m_assetHandles)[n];
				Volt::ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
				

				std::string handleString = std::format("{}", handle);
				ImGui::PushID(handleString.c_str());

				//Handle
				ImGui::TableNextColumn();
				ImGui::Text(handleString.c_str());

				if (!metadata.IsValid())
				{
					ImGui::TableNextColumn();
					ImGui::Text("ERROR");
					ImGui::TableNextColumn();
					ImGui::Text("ERROR");
					ImGui::TableNextColumn();
					ImGui::Text("ERROR");
					ImGui::PopID();
					continue;
				}

				//Type
				ImGui::TableNextColumn();
				ImGui::Text(metadata->type->GetName().data());
				if (CanVisualizeAssetType(metadata->type))
				{
					ImGui::SameLine();
					ImGui::Image(UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Visible)), ImVec2(ImGui::CalcTextSize("").y, ImGui::CalcTextSize("").y));
					if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
					{
						VisualizeAsset(metadata);
						ImGui::EndTooltip();
					}
				}
				//Path
				ImGui::TableNextColumn();
				if (metadata->HasFilepath())
				{
					ImGui::Text(metadata->filepath.string().c_str());
				}
				else
				{
					ImGui::Text("No Path Assigned");
				}

				//Load State
				ImGui::TableNextColumn();
				ImGui::Text(Volt::ToString(metadata->GetLoadState()).c_str());
				EditorIcon lockedIcon = EditorIcon::Unlocked;
				ImVec4 lockedColor = ImVec4(0.81f, 0.122f, 0.176f, 1.f);
				if (m_forceLoadedAssets.contains(metadata->handle))
				{
					lockedColor = ImVec4(0.f, 0.392f, 0.f, 1.f);
					lockedIcon = EditorIcon::Locked;
				}
				ImGui::SameLine();
				if (ImGui::ImageButton("##ForceLoad", UI::GetTextureID(EditorResources::GetEditorIcon(lockedIcon)), ImVec2(ImGui::CalcTextSize("").y, ImGui::CalcTextSize("").y), ImVec2(0, 0), ImVec2(1, 1), lockedColor))
				{
					if (m_forceLoadedAssets.contains(metadata->handle))
					{
						m_forceLoadedAssets.erase(metadata->handle);
					}
					else
					{
						AssetReference<Volt::Asset> asset;
						g_assetManager->TryGetTypelessAssetImmediately(metadata->handle, asset);
						VT_ENSURE(asset);
						m_forceLoadedAssets.insert({ metadata->handle, asset });
					}
				}
				UI::SimpleToolTip("Force Load");


				

				ImGui::PopID();
			}
		}
		clipper.End();

		ImGui::EndTable();
	}

}

void AssetRegistryPanel::OnOpen()
{
	m_assetChangedCallbackID = g_assetManager->RegisterAssetUpdatedCallback(AssetTypes::None,
	[this](Volt::AssetHandle assetHandle, Volt::AssetChangedState state)
	{
		QueueUpdateMetadata();
	});
}

void AssetRegistryPanel::OnClose()
{
	g_assetManager->UnregisterAssetUpdatedCallback(AssetTypes::None,m_assetChangedCallbackID);
}


bool AssetRegistryPanel::CanVisualizeAssetType(AssetType type) const
{
	if (type == AssetTypes::Texture)
	{
		return true;
	}

	return false;
}

void AssetRegistryPanel::VisualizeAsset(const Volt::ReadOnlyAssetMetadata& metadata)
{
	if (!CanVisualizeAssetType(metadata->type))
	{
		return;
	}

	if (!g_assetManager->IsAssetLoaded(metadata->handle))
	{
		ImGui::Text("Can only visualize loaded assets");
		return;
	}

	if (metadata->type == AssetTypes::Texture)
	{
		AssetReference<Volt::Texture2D> texture;
		g_assetManager->TryGetAssetIfLoaded(metadata->handle, texture);
		if (texture)
		{
			ImGui::Image(UI::GetTextureID(texture->GetImage()), ImVec2(128, 128));
		}
	}
}

void AssetRegistryPanel::OnSearchChanged()
{
	QueueUpdateMetadata();
}

void AssetRegistryPanel::QueueUpdateMetadata()
{
	m_updateQueued = true;
}

bool AssetRegistryPanel::DispatchUpdateMetadata()
{
	bool expected = false;
	const bool changed = m_updating.compare_exchange_strong(expected, true, std::memory_order::relaxed);
	if (!changed)
	{
		//could not dispatch since an update is already running
		return false;
	}
	Volt::TaskGraph taskGraph(Volt::ExecutionPriority::Latent);
	//copy the string since it might chagne while processing
	taskGraph.AddTask("Update Asset Registry Metadata", [search = m_searchString, this]()
	{
		if (m_wantsSwap)
		{
			m_wantsSwap.wait(false);
		}

		//target the one not in use
		Vector<Volt::AssetHandle>* targetAssetsPtr = &m_intermediateAssetHandles_1;
		if (targetAssetsPtr == m_assetHandles)
		{
			targetAssetsPtr = &m_intermediateAssetHandles_2;
		}

		targetAssetsPtr->clear();
		Volt::AssetRegistryIteratorFilter filter;
		g_assetManager->IterateAssetRegistryWithFilter(filter, [targetAssetsPtr, &search](Volt::ReadOnlyAssetMetadata metadata)
		{
			if (PassesFilter(metadata, search))
			{
				targetAssetsPtr->push_back(metadata->handle);
			}
			return true;
		});

		VT_ENSURE(m_updating == true);
		m_updating.exchange(false, std::memory_order::acq_rel);

		VT_ENSURE(m_wantsSwap == false);
		m_wantsSwap.exchange(true, std::memory_order::acq_rel);
	});
	taskGraph.Execute();
	return true;
}

void AssetRegistryPanel::SwapOpportunity()
{
	if (!m_wantsSwap)
	{
		return;
	}

	if (m_assetHandles == &m_intermediateAssetHandles_1)
	{
		m_assetHandles = &m_intermediateAssetHandles_2;
		m_intermediateAssetHandles_1.clear();
	}
	else
	{
		m_assetHandles = &m_intermediateAssetHandles_1;
		m_intermediateAssetHandles_2.clear();
	}

	m_wantsSwap.exchange(false, std::memory_order::acq_rel);
}

bool AssetRegistryPanel::PassesFilter(Volt::ReadOnlyAssetMetadata& metadata, std::string_view search)
{
	VT_ENSURE(metadata.IsValid());
	{
		std::string handleAsString = std::format("{}", metadata->handle);
		if (handleAsString.contains(search))
		{
			return true;
		}
	}
	if (Utility::ToLower(metadata->filepath.string()).contains(search))
	{
		return true;
	}

	return false;
}

