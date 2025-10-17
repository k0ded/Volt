#include "sbpch.h"
#include "Window/PrefabEditorPanel.h"

#include "Sandbox/Camera/EditorCameraController.h"
#include "Sandbox/Utility/EditorResources.h"

#include <Volt-Assets/MeshAsset.h>

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/SceneRenderer.h>
#include <Volt-CoreComponents/RenderingComponents.h>

#include <Volt-Scene/Scene.h>

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Core/Project/ProjectManager.h>

#include <InputModule/Input.h>

#include <AssetSystem/AssetManager.h>
#include <WindowModule/Events/WindowEvents.h>

#include <CoreUtilities/FileSystem.h>

PrefabEditorPanel::PrefabEditorPanel()
	: EditorWindow("Prefab Editor", true)
{
	RegisterListener<Volt::AppRenderEvent>(VT_BIND_EVENT_FN(PrefabEditorPanel::OnRenderEvent));

	myCameraController = CreateRef<EditorCameraController>(60.f, 1.f, 100000.f);
	//todo_fabian: this was disabled for now, make sure prefabs work
	//myScene = Volt::Scene::CreateDefaultScene("Prefab Editor", false);

	//myScene->Clear();

	mySceneViewPanel = CreateRef<SceneViewPanel>(myScene, "##PrefabEditor");
	myPropertiesPanel = CreateRef<PropertiesPanel>(myScene, mySceneRenderer, mySceneState, "##PrefabEditor");
}

void PrefabEditorPanel::UpdateMainContent()
{
	UpdateViewport();
	UpdateSceneView();
	UpdateProperties();

	//UpdateToolbar();
}

void PrefabEditorPanel::OpenAsset(Ref<Volt::Asset> asset)
{
	if (asset && asset->IsValid() && asset->GetType() == AssetTypes::Mesh)
	{
		myPreviewEntity.GetComponent<Volt::MeshComponent>().handle = asset->handle;
		myCurrentMesh = std::reinterpret_pointer_cast<Volt::MeshAsset>(asset);
		mySelectedSubMesh = 0;
	}
}

void PrefabEditorPanel::OnOpen()
{
	// Scene Renderer
	{
		Volt::SceneRendererCreateInfo spec{};
		spec.debugName = "Prefab Editor";
		spec.renderScene = myScene->GetRenderScene();

		//Volt::SceneRendererSettings settings{};
		//settings.enableGrid = true;

		mySceneRenderer = CreateRef<Volt::SceneRenderer>(spec);
	}
}

void PrefabEditorPanel::OnClose()
{
	mySceneRenderer = nullptr;
}

bool PrefabEditorPanel::OnRenderEvent(Volt::AppRenderEvent& e)
{
	mySceneRenderer->OnRenderEditor(myCameraController->GetCamera(), e.GetTimestep());
	return false;
}

void PrefabEditorPanel::UpdateViewport()
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0.f, 0.f });
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2{ 0.f, 0.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0.f, 0.f });

	ImGui::Begin("Viewport##prefabEditor");

	auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
	auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
	auto viewportOffset = ImGui::GetWindowPos();

	myPerspectiveBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
	myPerspectiveBounds[1] = { viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y };

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (myViewportSize != (*(glm::vec2*)&viewportSize) && viewportSize.x > 0 && viewportSize.y > 0 && !Volt::Input::IsMouseButtonDown(Volt::InputCode::Mouse_LB))
	{
		myViewportSize = { viewportSize.x, viewportSize.y };
		mySceneRenderer->Resize((uint32_t)myViewportSize.x, (uint32_t)myViewportSize.y);
		myCameraController->UpdateProjection((uint32_t)myViewportSize.x, (uint32_t)myViewportSize.y);
	}

	ImGui::Image(UI::GetTextureID(mySceneRenderer->GetFinalImage()), viewportSize);
	ImGui::End();
	ImGui::PopStyleVar(3);
}

void PrefabEditorPanel::UpdateSceneView()
{
	if (mySceneViewPanel->Begin())
	{
		mySceneViewPanel->UpdateMainContent();
		mySceneViewPanel->End();
		mySceneViewPanel->UpdateContent();
	}
}

void PrefabEditorPanel::UpdateProperties()
{
	if (myPropertiesPanel->Begin())
	{
		myPropertiesPanel->UpdateMainContent();
		myPropertiesPanel->End();
		myPropertiesPanel->UpdateContent();
	}
}

void PrefabEditorPanel::UpdateToolbar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 2.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.f, 0.f));
	UI::ScopedColor button(ImGuiCol_Button, { 0.f, 0.f, 0.f, 0.f });
	UI::ScopedColor hovered(ImGuiCol_ButtonHovered, { 0.3f, 0.305f, 0.31f, 0.5f });
	UI::ScopedColor active(ImGuiCol_ButtonActive, { 0.5f, 0.505f, 0.51f, 0.5f });

	ImGui::Begin("##toolbarPrefabEditor", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	if (UI::ImageButton("##Save", UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Save)), { myButtonSize, myButtonSize }))
	{
		if (myCurrentMesh)
		{
			SaveCurrentMesh();
		}
	}

	ImGui::SameLine();

	if (UI::ImageButton("##Load", UI::GetTextureID(EditorResources::GetEditorIcon(EditorIcon::Open)), { myButtonSize, myButtonSize }))
	{
		const std::filesystem::path prefabPath = FileSystem::OpenFileDialogue({{ "Prefab (*.vtprefab)", "vtchr" }}, Volt::ProjectManager::GetAssetsDirectory());
		if (!prefabPath.empty() && FileSystem::Exists(prefabPath))
		{
			myCurrentMesh = Volt::AssetManager::GetAsset<Volt::MeshAsset>(prefabPath);
			myPreviewEntity.GetComponent<Volt::MeshComponent>().handle = myCurrentMesh->handle;
			mySelectedSubMesh = 0;
		}
	}
	ImGui::PopStyleVar(2);
	ImGui::End();
}

void PrefabEditorPanel::UpdateMeshList()
{
	ImGui::Begin("Mesh List##prefabEditor");

	if (!myCurrentMesh)
	{
		ImGui::End();
		return;
	}

	for (uint32_t i = 0; const auto & subMesh : myCurrentMesh->GetMesh()->GetSubMeshes())
	{
		std::string id = subMesh.name + "##subMesh" + std::to_string(i);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		if (mySelectedSubMesh == i)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		ImGui::TreeNodeEx(id.c_str(), flags);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			mySelectedSubMesh = i;
		}

		i++;
	}

	ImGui::End();
}

void PrefabEditorPanel::SaveCurrentMesh()
{
	if (!myCurrentMesh)
	{
		return;
	}

	const auto filesystemPath = Volt::AssetManager::GetFilesystemPath(myCurrentMesh->handle);
	const auto& metadata = Volt::AssetManager::GetMetadataFromHandle(myCurrentMesh->handle);

	if (!FileSystem::IsWriteable(filesystemPath))
	{
		UI::Notify(UI::NotificationType::Error, "Unable to save Mesh!", std::format("Unable to save mesh {0}! It is not writeable!", metadata.filePath.string()));
		return;
	}

	//if (!Volt::MeshCompiler::TryCompile(myCurrentMesh, metadata.filePath, myCurrentMesh->GetMaterialTable()))
	//{
	//	UI::Notify(UI::NotificationType::Error, "Unable to save Mesh!", std::format("Unable to save mesh {0}!", metadata.filePath.string()));
	//}
	//else
	//{
	//	UI::Notify(UI::NotificationType::Success, "Saved Mesh!", std::format("Mesh {0} was saved successfully", metadata.filePath.string()));
	//}
}
