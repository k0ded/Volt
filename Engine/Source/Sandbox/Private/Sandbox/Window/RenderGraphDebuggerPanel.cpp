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
	constexpr Array<StringView, 6> RenderGraphPassFlagsNames =
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
						String flagsString;
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
									flagsString += " | " + String(RenderGraphPassFlagsNames[i]);
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

					if (ImGui::CollapsingHeader("Render Targets", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
					{
						for (uint32_t resourceId : selectedPass.renderTargets)
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
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_Hideable |
			ImGuiTableFlags_HighlightHoveredColumn;

		constexpr ImGuiTableColumnFlags ColumnFlags =
			ImGuiTableColumnFlags_AngledHeader |
			ImGuiTableColumnFlags_WidthFixed;

		constexpr ImU32 WriteColor = IM_COL32(212, 51, 51, 255);
		constexpr ImU32 ReadColor = IM_COL32(51, 212, 83, 255);
		constexpr ImU32 NoAccessColor = IM_COL32(128, 128, 128, 255);
		constexpr ImU32 ReferenceLineColor = IM_COL32(150, 150, 150, 200);

		constexpr size_t MaxPassNameLength = 20;
		constexpr float ColumnWidth = 30.f;
		constexpr float RowHeight = 20.f;

		UI::ScopedStyleFloat2 cellPadding{ ImGuiStyleVar_CellPadding, { 0.f } };
		UI::ScopedStyleFloat headerAngle{ ImGuiStyleVar_TableAngledHeadersAngle, glm::radians(50.f)};

		const int32_t numColumns = numRenderPasses + 1;
		if (ImGui::BeginTable("LifetimeTable", numColumns, TableFlags))
		{
			ImGuiWindow* tableWindow = ImGui::GetCurrentWindow();

			ImGui::TableSetupColumn("Resources", ImGuiTableColumnFlags_NoHide);

			for (int32_t i = 0; i < numRenderPasses; ++i)
			{
				String passName = renderPasses[i].passName;
				if (passName.size() > MaxPassNameLength)
				{
					passName = passName.substr(0, MaxPassNameLength - 3);
					passName += "...";
				}

				ImGui::TableSetupColumn(passName.c_str(), ColumnFlags, ColumnWidth);
			}
			ImGui::TableSetupScrollFreeze(1, 2);
			ImGui::TableAngledHeadersRow();
			ImGui::TableHeadersRow();

			auto DrawResourceRow = [tableWindow, numColumns](const RenderGraphDebugger::RenderGraphResource& resource, int32_t row)
			{
				ImGui::PushID(row);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted(resource.name.c_str());

				for (int32_t column = 1; column < static_cast<int32_t>(resource.lastUsagePass) + 1; ++column)
				{
					if (ImGui::TableSetColumnIndex(column))
					{
						ImVec2 startPos = tableWindow->DC.CursorPos;
						ImVec2 lineStartPos = startPos + ImVec2(0.f, RowHeight * 0.5f);

						constexpr float LineSpacing = 2.f;
						constexpr int32_t NumLines = 3;
						const float lineWidth = (ColumnWidth / NumLines) - LineSpacing;

						for (int32_t lineIndex = 0; lineIndex < NumLines; ++lineIndex)
						{
							ImVec2 lineEndPos = lineStartPos + ImVec2(lineWidth, 0.f);
							tableWindow->DrawList->AddLine(lineStartPos, lineEndPos, ReferenceLineColor);
							lineStartPos = lineEndPos + ImVec2(LineSpacing, 0.f);
						}
					}
				}

				for (int32_t column = resource.firstUsagePass + 1; column <= static_cast<int32_t>(resource.lastUsagePass) + 1; ++column)
				{
					if (ImGui::TableSetColumnIndex(column))
					{
						ImGui::PushID(column);

						ImU32 color = NoAccessColor;

						auto passAccessIt = resource.passAccesses.find_with_predicate([column](const auto& passAccess) 
						{ 
							return static_cast<int32_t>(passAccess.passIndex) == column - 1; 
						});

						if (passAccessIt != resource.passAccesses.end())
						{
							if (passAccessIt->isRead)
							{
								color = ReadColor;
							}
							else
							{
								color = WriteColor;
							}
						}

						ImVec2 startPos = tableWindow->DC.CursorPos;
						tableWindow->DrawList->AddRectFilled(startPos, startPos + ImVec2(ColumnWidth, RowHeight), color);

						ImGui::PopID();
					}
				}

				ImGui::PopID();
			};

			for (int32_t row = 0; row < numTransientResources; ++row)
			{
				auto it = transientResources.begin() + row;
				const RenderGraphDebugger::RenderGraphResource& resource = it->second;
				DrawResourceRow(resource, row);
			}

			for (int32_t row = 0; row < numExternalResources; ++row)
			{
				auto it = externalResources.begin() + row;
				const RenderGraphDebugger::RenderGraphResource& resource = it->second;
				DrawResourceRow(resource, row + numTransientResources);
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
