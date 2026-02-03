#include "sbpch.h"
#include "Utility/EditorResources.h"

#include <Volt-Assets/MeshAsset.h>

#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>
#include <Volt-Renderer/ShapeLibrary.h>
#include <Volt-Renderer/Texture/Texture2D.h>


#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/SourceAssetManager.h>

#include <Volt-Scene/AssetTypes.h>
#include <Volt-Animation/Assets/AssetTypes.h>

void EditorResources::Initialize()
{
	// Asset icons
	{
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_material.vtasset", &m_assetIcons[AssetTypes::Material]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_mesh.vtasset", &m_assetIcons[AssetTypes::Mesh]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_meshSource.vtasset", &m_assetIcons[AssetTypes::MeshSource]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_navmesh.vtasset", &m_assetIcons[AssetTypes::NavMesh]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_skeleton.vtasset", &m_assetIcons[AssetTypes::Skeleton]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_animation.vtasset", &m_assetIcons[AssetTypes::Animation]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_scene.vtasset", &m_assetIcons[AssetTypes::Scene]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_prefab.vtasset", &m_assetIcons[AssetTypes::Prefab]);
		TryLoadIcon("Editor/Textures/Icons/AssetIcons/icon_behaviorTree.vtasset", &m_assetIcons[AssetTypes::BehaviorGraph]);
	}

	// Editor Icons
	{
		TryLoadIcon("Editor/Textures/Icons/icon_directory.vtasset", &m_editorIcons[EditorIcon::Directory]);
		TryLoadIcon("Editor/Textures/Icons/icon_back.vtasset", &m_editorIcons[EditorIcon::Back]);
		TryLoadIcon("Editor/Textures/Icons/icon_reload.vtasset", &m_editorIcons[EditorIcon::Reload]);
		TryLoadIcon("Editor/Textures/Icons/icon_search.vtasset", &m_editorIcons[EditorIcon::Search]);
		TryLoadIcon("Editor/Textures/Icons/icon_settings.vtasset", &m_editorIcons[EditorIcon::Settings]);
		TryLoadIcon("Editor/Textures/Icons/icon_play.vtasset", &m_editorIcons[EditorIcon::Play]);
		TryLoadIcon("Editor/Textures/Icons/icon_stop.vtasset", &m_editorIcons[EditorIcon::Stop]);
		TryLoadIcon("Editor/Textures/Icons/icon_file.vtasset", &m_editorIcons[EditorIcon::GenericFile]);
		TryLoadIcon("Editor/Textures/Icons/icon_save.vtasset", &m_editorIcons[EditorIcon::Save]);
		TryLoadIcon("Editor/Textures/Icons/icon_open.vtasset", &m_editorIcons[EditorIcon::Open]);
		TryLoadIcon("Editor/Textures/Icons/icon_add.vtasset", &m_editorIcons[EditorIcon::Add]);
		TryLoadIcon("Editor/Textures/Icons/icon_filter.vtasset", &m_editorIcons[EditorIcon::Filter]);

		TryLoadIcon("Editor/Textures/Icons/icon_unlocked.vtasset", &m_editorIcons[EditorIcon::Unlocked]);
		TryLoadIcon("Editor/Textures/Icons/icon_locked.vtasset", &m_editorIcons[EditorIcon::Locked]);

		TryLoadIcon("Editor/Textures/Icons/icon_hidden.vtasset", &m_editorIcons[EditorIcon::Hidden]);
		TryLoadIcon("Editor/Textures/Icons/icon_visible.vtasset", &m_editorIcons[EditorIcon::Visible]);

		TryLoadIcon("Editor/Textures/Icons/icon_entityGizmo.vtasset", &m_editorIcons[EditorIcon::EntityGizmo]);

		TryLoadIcon("Editor/Textures/Icons/icon_lightGizmo.vtasset", &m_editorIcons[EditorIcon::LightGizmo]);

		TryLoadIcon("Editor/Textures/Icons/icon_localSpace.vtasset", &m_editorIcons[EditorIcon::LocalSpace]);
		TryLoadIcon("Editor/Textures/Icons/icon_worldSpace.vtasset", &m_editorIcons[EditorIcon::WorldSpace]);

		TryLoadIcon("Editor/Textures/Icons/icon_snapRotation.vtasset", &m_editorIcons[EditorIcon::SnapRotation]);
		TryLoadIcon("Editor/Textures/Icons/icon_snapScale.vtasset", &m_editorIcons[EditorIcon::SnapScale]);
		TryLoadIcon("Editor/Textures/Icons/icon_snapToGrid.vtasset", &m_editorIcons[EditorIcon::SnapGrid]);
		TryLoadIcon("Editor/Textures/Icons/icon_showGizmo.vtasset", &m_editorIcons[EditorIcon::ShowGizmos]);

		TryLoadIcon("Editor/Textures/Icons/icon_fullscreenOnPlay.vtasset", &m_editorIcons[EditorIcon::FullscreenOnPlay]);

		TryLoadIcon("Editor/Textures/Icons/icon_getMaterial.vtasset", &m_editorIcons[EditorIcon::GetMaterial]);
		TryLoadIcon("Editor/Textures/Icons/icon_setMaterial.vtasset", &m_editorIcons[EditorIcon::SetMaterial]);

		TryLoadIcon("Editor/Textures/Icons/icon_close.vtasset", &m_editorIcons[EditorIcon::Close]);
		TryLoadIcon("Editor/Textures/Icons/icon_minimize.vtasset", &m_editorIcons[EditorIcon::Minimize]);
		TryLoadIcon("Editor/Textures/Icons/icon_maximize.vtasset", &m_editorIcons[EditorIcon::Maximize]);
		TryLoadIcon("Editor/Textures/Icons/icon_windowize.vtasset", &m_editorIcons[EditorIcon::Windowize]);

		TryLoadIcon("Editor/Textures/Icons/icon_paintBrush.vtasset", &m_editorIcons[EditorIcon::Paint]);
		TryLoadIcon("Editor/Textures/Icons/icon_click.vtasset", &m_editorIcons[EditorIcon::Select]);
		TryLoadIcon("Editor/Textures/Icons/icon_dentalFilling.vtasset", &m_editorIcons[EditorIcon::Fill]);
		TryLoadIcon("Editor/Textures/Icons/icon_swap.vtasset", &m_editorIcons[EditorIcon::Swap]);
		TryLoadIcon("Editor/Textures/Icons/icon_remove.vtasset", &m_editorIcons[EditorIcon::Remove]);

		TryLoadIcon("Editor/Textures/Icons/icon_volt.vtasset", &m_editorIcons[EditorIcon::Volt]);
	}

	// Meshes
	{
		m_editorMeshes[EditorMesh::Cube] = TryLoadMesh("Engine/Meshes/Primitives/SM_Cube.vtasset");
		m_editorMeshes[EditorMesh::Capsule] = TryLoadMesh("Engine/Meshes/Primitives/SM_Capsule.vtasset");
		m_editorMeshes[EditorMesh::Cone] = TryLoadMesh("Engine/Meshes/Primitives/SM_Cone.vtasset");
		m_editorMeshes[EditorMesh::Cylinder] = TryLoadMesh("Engine/Meshes/Primitives/SM_Cylinder.vtasset");
		m_editorMeshes[EditorMesh::Plane] = TryLoadMesh("Engine/Meshes/Primitives/SM_Plane.vtasset");
		m_editorMeshes[EditorMesh::Sphere] = TryLoadMesh("Engine/Meshes/Primitives/SM_Sphere.vtasset");
		m_editorMeshes[EditorMesh::Arrow] = TryLoadMesh("Editor/Meshes/Arrow/3dpil_.vtasset");
		m_editorMeshes[EditorMesh::Camera] = TryLoadMesh("Editor/Meshes/Gizmos/SM_Camera_Gizmo_.vtasset");
	}
}

void EditorResources::Shutdown()
{
	m_assetIcons.clear();
	m_editorIcons.clear();
	m_editorMeshes.clear();
}

RefPtr<Volt::RHI::Image> EditorResources::GetAssetIcon(AssetType type)
{
	if (!m_assetIcons.contains(type))
	{
		return nullptr;
	}

	return m_assetIcons.at(type);
}

RefPtr<Volt::RHI::Image> EditorResources::GetEditorIcon(EditorIcon icon)
{
	if (!m_editorIcons.contains(icon))
	{
		return Volt::Renderer::GetDefaultResources().white1x1;
	}

	return m_editorIcons.at(icon);
}

Ref<Volt::Mesh> EditorResources::GetEditorMesh(EditorMesh mesh)
{
	if (!m_editorMeshes.contains(mesh))
	{
		return Volt::ShapeLibrary::GetCube();
	}

	return m_editorMeshes.at(mesh);
}

void EditorResources::TryLoadIcon(const std::filesystem::path& path, RefPtr<Volt::RHI::Image>* outTexture)
{
	AssetReference<Volt::Texture2D> texture;

	if (g_assetManager->TryGetAssetImmediately(path, texture))
	{
		*outTexture = texture->GetImage();
	}
	else
	{
		*outTexture = Volt::Renderer::GetDefaultResources().white1x1;
	}
}

Ref<Volt::Mesh> EditorResources::TryLoadMesh(const std::filesystem::path& path)
{
	AssetReference<Volt::MeshAsset> meshAsset;

	Ref<Volt::Mesh> mesh;

	if (g_assetManager->TryGetAssetImmediately(path, meshAsset))
	{
		mesh = meshAsset->GetMesh();
	}
	else
	{
		mesh = Volt::ShapeLibrary::GetCube();
	}

	return mesh;
}
