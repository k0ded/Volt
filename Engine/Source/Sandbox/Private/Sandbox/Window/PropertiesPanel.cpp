#include "sbpch.h"
#include "Window/PropertiesPanel.h"

#include "Sandbox/Utility/SelectionManager.h"
#include "Sandbox/Utility/EditorUtilities.h"
#include "Sandbox/Utility/Theme.h"
#include "Sandbox/Utility/ComponentPropertyUtilities.h"
#include "Sandbox/Utility/PremadeCommands.h"

#include "Sandbox/Sandbox.h"
#include "Sandbox/UserSettingsManager.h"
#include "Sandbox/EditorCommandStack.h"

#include <Volt-CoreComponents/LightComponents.h>

#include <EntitySystem/ComponentRegistry.h>

#include <InputModule/Input.h>
#include <InputModule/InputCodes.h>
#include <InputModule/MouseButtonCodes.h>

#include <Volt-Application/UI/UIUtility.h>

PropertiesPanel::PropertiesPanel(AssetReference<Volt::Scene>& currentScene, Ref<Volt::SceneRenderer>& currentSceneRenderer, SceneState& sceneState, const std::string& id)
	: EditorWindow("Properties", false, id), myCurrentScene(currentScene), myCurrentSceneRenderer(currentSceneRenderer), mySceneState(sceneState)
{
	Open();
	myMaxEventListSize = 20;
	myLastValue = std::make_shared<PropertyEvent>();
}

void PropertiesPanel::UpdateMainContent()
{
	if (myMidEvent == true)
	{
		if (ImGui::IsMouseReleased(0))
		{
			myMidEvent = false;
		}
	}

	if (!SelectionManager::IsAnySelected())
	{
		return;
	}

	const bool singleSelected = !(SelectionManager::GetSelectedCount() > 1);
	auto firstEntity = myCurrentScene->GetEntityFromID(SelectionManager::GetSelectedEntities().front());
	const auto& entities = SelectionManager::GetSelectedEntities();

	if (singleSelected)
	{
		if (firstEntity.HasComponent<Volt::TagComponent>())
		{
			auto& tag = firstEntity.GetComponent<Volt::TagComponent>();
			if (UI::InputText("Name", tag.tag))
			{
				EditorUtils::MarkEntityAsEdited(*myCurrentScene, firstEntity);
			}
		}
	}
	else
	{
		static std::string inputText = "...";

		std::string firstName;
		bool sameName = true;

		if (firstEntity.HasComponent<Volt::TagComponent>())
		{
			firstName = firstEntity.GetTag();
		}

		for (auto& id : SelectionManager::GetSelectedEntities())
		{
			Volt::Entity entity = myCurrentScene->GetEntityFromID(id);

			if (entity.HasComponent<Volt::TagComponent>())
			{
				auto& tag = entity.GetComponent<Volt::TagComponent>();
				if (tag.tag != firstName)
				{
					sameName = false;
					break;
				}
			}
		}

		if (sameName)
		{
			inputText = firstName;
		}
		else
		{
			inputText = "...";
		}

		UI::PushID();
		if (UI::InputText("Name", inputText))
		{
			for (auto& id : SelectionManager::GetSelectedEntities())
			{
				Volt::Entity entity = myCurrentScene->GetEntityFromID(id);

				if (entity.HasComponent<Volt::TagComponent>())
				{
					entity.GetComponent<Volt::TagComponent>().tag = inputText;
					EditorUtils::MarkEntityAsEdited(*myCurrentScene, entity);
				}
			}
		}
		UI::PopID();
	}

	// Transform
	{
		UI::PushID();
		if (UI::BeginProperties("Transform"))
		{
			auto& entityId = SelectionManager::GetSelectedEntities().front();
			Volt::Entity entity = myCurrentScene->GetEntityFromID(entityId);

			if (entity.HasComponent<Volt::TransformComponent>())
			{
				auto& transform = entity.GetComponent<Volt::TransformComponent>();

				if (UI::PropertyAxisColor("Position", transform.position, 0.f))
				{
					if (myMidEvent == false)
					{
						Ref<ValueCommand<glm::vec3>> command = CreateRef<ValueCommand<glm::vec3>>(&transform.position, transform.position);
						EditorCommandStack::PushUndo(command);
						myMidEvent = true;
					}

					for (auto& entId : entities)
					{
						Volt::Entity ent = myCurrentScene->GetEntityFromID(entId);
						ent.SetLocalPosition(transform.position);
						myCurrentScene->InvalidateEntityTransform(entId);

						EditorUtils::MarkEntityAndChildrenAsEdited(*myCurrentScene, ent);
					}
				}

				const glm::vec3 originalEuler = glm::eulerAngles(transform.rotation);
				glm::vec3 rotDegrees = glm::degrees(originalEuler);

				if (UI::PropertyAxisColor("Rotation", rotDegrees, 0.f))
				{
					transform.rotation = glm::quat{ glm::radians(rotDegrees) };

					if (myMidEvent == false)
					{
						Ref<ValueCommand<glm::quat>> command = CreateRef<ValueCommand<glm::quat>>(&transform.rotation, transform.rotation);
						EditorCommandStack::PushUndo(command);
						myMidEvent = true;
					}

					for (auto& entId : entities)
					{
						Volt::Entity ent = myCurrentScene->GetEntityFromID(entId);
						ent.SetLocalRotation(transform.rotation);
						myCurrentScene->InvalidateEntityTransform(entId);

						EditorUtils::MarkEntityAsEdited(*myCurrentScene, ent);
					}
				}

				if (UI::PropertyAxisColor("Scale", transform.scale, 1.f))
				{
					if (myMidEvent == false)
					{
						Ref<ValueCommand<glm::vec3>> command = CreateRef<ValueCommand<glm::vec3>>(&transform.scale, transform.scale);
						EditorCommandStack::PushUndo(command);
						myMidEvent = true;
					}

					for (auto& entId : entities)
					{
						Volt::Entity ent = myCurrentScene->GetEntityFromID(entId);
						ent.SetLocalScale(transform.scale);
						myCurrentScene->InvalidateEntityTransform(entId);

						EditorUtils::MarkEntityAsEdited(*myCurrentScene, ent);
					}
				}
			}

			UI::EndProperties();
		}
		UI::PopID();
	}

	if (singleSelected)
	{
		const auto id = SelectionManager::GetSelectedEntities().front();
		Volt::Entity entity = myCurrentScene->GetEntityFromID(id);

		ComponentPropertyUtility::DrawComponents(*myCurrentScene, entity);
	}

	ImGui::PushStyleColor(ImGuiCol_Button, { 0.2f, 0.2f, 0.2f, 1.f });
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.f);

	constexpr float buttonHeight = 22.f;

	if (ImGui::Button("Add Component", { ImGui::GetContentRegionAvail().x, buttonHeight }))
	{
		myComponentSearchQuery = "";
		myActivateComponentSearch = true;
		UI::OpenPopup("AddComponent");
	}

	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	AddComponentPopup();
}

void PropertiesPanel::AddComponentPopup()
{
	ImGui::SetNextWindowSize({ 250.f, 500.f });
	if (UI::BeginPopup("AddComponent" + m_id, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		Vector<std::string> componentNames;
		std::unordered_map<std::string, VoltGUID> nameToGUIDMap;

		const auto& componentRegistry = Volt::ComponentRegistry::Get().GetRegistry();
		for (const auto& [guid, typeDesc] : componentRegistry)
		{
			if (typeDesc->GetValueType() == Volt::ValueType::Component)
			{
				const Volt::IComponentTypeDesc* compDesc = reinterpret_cast<const Volt::IComponentTypeDesc*>(typeDesc);
				if (compDesc->IsHidden())
				{
					continue;
				}

				const std::string strLabel = std::string(compDesc->GetLabel());

				componentNames.emplace_back(strLabel);
				nameToGUIDMap[strLabel] = guid;
			}
		}

		std::sort(componentNames.begin(), componentNames.end());

		// Search bar
		{
			bool t;
			EditorUtils::SearchBar(myComponentSearchQuery, t, myActivateComponentSearch);
			if (myActivateComponentSearch)
			{
				myActivateComponentSearch = false;
			}
		}

		if (!myComponentSearchQuery.empty())
		{
			componentNames = UI::GetEntriesMatchingQuery(myComponentSearchQuery, componentNames);
		}

		{
			UI::ScopedColor background{ ImGuiCol_ChildBg, EditorTheme::DarkGreyBackground };
			ImGui::BeginChild("scrolling", ImGui::GetContentRegionAvail());

			for (const auto& label : componentNames)
			{
				const auto compGuid = nameToGUIDMap.at(label);

				Volt::Entity frontEntity = myCurrentScene->GetEntityFromID(SelectionManager::GetSelectedEntities().front());
				if (!frontEntity.HasComponent(compGuid))
				{
					UI::ShiftCursor(4.f, 0.f);
					UI::RenderMatchingTextBackground(myComponentSearchQuery, label, EditorTheme::MatchingTextBackground);
					if (ImGui::MenuItem(label.c_str()))
					{
						for (auto& ent : SelectionManager::GetSelectedEntities())
						{
							auto entity = myCurrentScene->GetEntityFromID(ent);

							if (!Volt::ComponentRegistry::Helpers::HasComponentWithGUID(compGuid, myCurrentScene->GetEntityScene().GetRegistry(), entity))
							{
								Volt::ComponentRegistry::Helpers::AddComponentWithGUID(compGuid, myCurrentScene->GetEntityScene().GetRegistry(), entity);

								const Volt::IComponentTypeDesc* componentTypeDesc = reinterpret_cast<const Volt::IComponentTypeDesc*>(Volt::ComponentRegistry::Get().GetTypeDescFromGUID(compGuid));
								if (componentTypeDesc)
								{
									componentTypeDesc->OnInitialize(entity);
								}

								EditorUtils::MarkEntityAsEdited(*myCurrentScene, entity);
							}
						}

						ImGui::CloseCurrentPopup();
					}
				}
			}

			ImGui::EndChild();
		}

		UI::EndPopup();
	}
}
