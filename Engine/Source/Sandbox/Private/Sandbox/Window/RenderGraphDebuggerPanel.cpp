#include "sbpch.h"
#include "Sandbox/Window/RenderGraphDebuggerPanel.h"

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Renderer/SceneRenderer.h>

using namespace Volt;

RenderGraphDebuggerPanel::RenderGraphDebuggerPanel(Ref<Volt::SceneRenderer>& sceneRenderer)
	: EditorWindow("Render Graph Debugger"), m_sceneRenderer(sceneRenderer)
{
}

void RenderGraphDebuggerPanel::UpdateMainContent()
{
	constexpr Array<std::string_view, 6> RenderGraphPassFlagsNames =
	{
		"NeverCull",
		"Compute",
		"Clear",
		"Raster",
		"Copy"
	};

	if (!m_sceneRenderer)
	{
		UI::ScopedFont font(UI::FontType::Regular, 90.f);
		ImGui::Text("No RenderGraph Running.");
		return;
	}

	const auto& renderGraphDebugger = m_sceneRenderer->GetRenderGraphDebugger();

	ArrayView<RenderGraphDebugger::RenderGraphPass> renderPasses = renderGraphDebugger.GetPasses();
	const RenderGraphDebugger::RenderGraphResourcesMap& transientResources = renderGraphDebugger.GetTransientResources();
	const RenderGraphDebugger::RenderGraphResourcesMap& externalResources = renderGraphDebugger.GetExternalResources();

	const int32_t numRenderPasses = static_cast<int32_t>(renderPasses.size());
	const int32_t numTransientResources = static_cast<int32_t>(transientResources.size());
	const int32_t numExternalResources = static_cast<int32_t>(externalResources.size());

	if (ImGui::CollapsingHeader("Render Passes"))
	{
		static int32_t selectedRenderPassIndex = -1;

		if (ImGui::BeginTable("PassesTable", 2))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			// Render Pass list
			if (ImGui::BeginChild("Scrollable"))
			{
				for (int32_t i = 0; i < numRenderPasses; i++)
				{
					ImGui::PushID(i);

					if (ImGui::Selectable(renderPasses[i].passName.c_str()))
					{
						selectedRenderPassIndex = i;
					}

					ImGui::PopID();
				}
			}
			ImGui::EndChild();

			ImGui::TableNextColumn();

			// Selected pass info
			{
				if (selectedRenderPassIndex != -1)
				{
					const RenderGraphDebugger::RenderGraphPass& selectedPass = renderPasses[selectedRenderPassIndex];

					ImGui::TextUnformatted("Pass Info");
					ImGui::Text("Is Culled: %s", selectedPass.isCulled ? "true" : "false");

					{
						std::string flagsString;
						const uint32_t flagBits = static_cast<uint32_t>(selectedPass.passFlags);

						for (size_t i = 0; i < RenderGraphPassFlagsNames.size(); ++i)
						{
							if ((flagBits & (1 << i)) != 0)
							{
								if (flagsString.empty())
								{
									flagsString = RenderGraphPassFlagsNames[i];
								}
								else
								{
									flagsString += " | " + std::string(RenderGraphPassFlagsNames[i]);
								}
							}
						}

						ImGui::Text("Pass Flags: %s", flagsString.c_str());
					}

					if (ImGui::CollapsingHeader("Resource Reads", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (uint32_t resourceId : selectedPass.resourceReads)
						{
							if (transientResources.contains(resourceId))
							{
								const RenderGraphDebugger::RenderGraphResource& resource = transientResources.at(resourceId);
								ImGui::TextUnformatted(resource.name.c_str());
							}
							else
							{
								const RenderGraphDebugger::RenderGraphResource& resource = externalResources.at(resourceId);
								ImGui::TextUnformatted(resource.name.c_str());
							}
						}
					}

					if (ImGui::CollapsingHeader("Resource Writes", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (uint32_t resourceId : selectedPass.resourceWrites)
						{
							if (transientResources.contains(resourceId))
							{
								const RenderGraphDebugger::RenderGraphResource& resource = transientResources.at(resourceId);
								ImGui::TextUnformatted(resource.name.c_str());
							}
							else
							{
								const RenderGraphDebugger::RenderGraphResource& resource = externalResources.at(resourceId);
								ImGui::TextUnformatted(resource.name.c_str());
							}
						}
					}
				}
			}

			ImGui::EndTable();
		}
	}

	if (ImGui::CollapsingHeader("Resource Lifetimes"))
	{
		constexpr ImGuiTableFlags TableFlags =
			ImGuiTableFlags_SizingFixedFit |
			ImGuiTableFlags_ScrollX |
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_Hideable |
			ImGuiTableFlags_HighlightHoveredColumn |
			ImGuiTableFlags_NoClip;

		constexpr ImGuiTableColumnFlags ColumnFlags =
			ImGuiTableColumnFlags_AngledHeader |
			ImGuiTableColumnFlags_WidthFixed;

		constexpr size_t MaxPassNameLength = 15;

		constexpr float ColumnWidth = 30.f;

		constexpr float RowHeight = 20.f;

		UI::ScopedStyleFloat2 cellPadding{ ImGuiStyleVar_CellPadding, { 0.f } };

		if (ImGui::BeginTable("LifetimeTable", numRenderPasses, TableFlags))
		{

			ImGuiWindow* tableWindow = ImGui::GetCurrentWindow();

			for (int32_t i = 0; i < numRenderPasses; ++i)
			{
				std::string passName = renderPasses[i].passName;
				if (passName.size() > MaxPassNameLength)
				{
					passName = passName.substr(0, MaxPassNameLength - 3);
					passName += "...";
				}

				ImGui::TableSetupColumn(passName.c_str(), ColumnFlags, ColumnWidth);
			}
			//ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableAngledHeadersRow(); // Draw angled headers for all columns with the ImGuiTableColumnFlags_AngledHeader flag.

			const ImU32 BufferColor = IM_COL32(86, 133, 165, 255);
			const ImU32 TextureColor = IM_COL32(93, 186, 143, 255);

			for (int32_t row = 0; row < numTransientResources; ++row)
			{
				ImGui::TableNextRow();

				auto it = transientResources.begin() + row;
				const RenderGraphDebugger::RenderGraphResource& resource = it->second;

				{
					ImGui::TableSetColumnIndex(resource.firstUsagePass);
					ImVec2 startPos = tableWindow->DC.CursorPos;

					ImGui::TableSetColumnIndex(resource.lastUsagePass);
					ImVec2 endPos = tableWindow->DC.CursorPos + ImVec2(ColumnWidth, RowHeight);

					ImU32 color = 0;

					if (resource.resourceType == RGResourceType::Buffer ||
						resource.resourceType == RGResourceType::UniformBuffer)
					{
						color = BufferColor;
					}
					else
					{
						color = TextureColor;
					}

					tableWindow->DrawList->AddRectFilled(startPos, endPos, color);

					const ImVec2 textSize = ImGui::CalcTextSize(resource.name.c_str());
					const ImVec2 textOffset = (startPos + ImVec2(endPos.x, endPos.y - RowHeight)) * 0.5f - ImVec2(textSize.x * 0.5f, 0.f);

					tableWindow->DrawList->AddText(textOffset, IM_COL32(0, 0, 0, 255), resource.name.c_str());

					ImGui::Dummy(ImVec2(ColumnWidth, RowHeight));
				}
			}

			for (int32_t row = 0; row < numExternalResources; ++row)
			{
				ImGui::TableNextRow();

				auto it = externalResources.begin() + row;
				const RenderGraphDebugger::RenderGraphResource& resource = it->second;

				{
					ImGui::TableSetColumnIndex(0);
					ImVec2 startPos = tableWindow->DC.CursorPos;

					ImGui::TableSetColumnIndex(numRenderPasses - 1);
					ImVec2 endPos = tableWindow->DC.CursorPos + ImVec2(ColumnWidth, RowHeight);

					ImU32 color = 0;

					if (resource.resourceType == RGResourceType::Buffer ||
						resource.resourceType == RGResourceType::UniformBuffer)
					{
						color = BufferColor;
					}
					else
					{
						color = TextureColor;
					}

					tableWindow->DrawList->AddRectFilled(startPos, endPos, color);

					const ImVec2 textSize = ImGui::CalcTextSize(resource.name.c_str());
					const ImVec2 textOffset = (startPos + ImVec2(endPos.x, endPos.y - RowHeight)) * 0.5f - ImVec2(textSize.x * 0.5f, 0.f);

					tableWindow->DrawList->AddText(textOffset, IM_COL32(0, 0, 0, 255), resource.name.c_str());

					ImGui::Dummy(ImVec2(ColumnWidth, RowHeight));
				}
			}

			ImGui::EndTable();
		}
	}

	renderGraphDebugger.WaitForFinishedExecution();
}

void RenderGraphDebuggerPanel::UpdateContent()
{
	ImGui::ShowDemoWindow();
}
