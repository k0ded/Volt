#pragma once

#include <AssetSystem/AssetType.h>

#include <RHIModule/Images/Image.h>

namespace Volt
{
	class Texture2D;
	class Mesh;
}

enum class EditorIcon
{
	Directory,
	Reload,
	Back,
	Settings,
	Search,
	Play,
	Stop,
	GenericFile,
	Save,
	Open,
	Add,
	Filter,

	Locked,
	Unlocked,
	Visible,
	Hidden,

	EntityGizmo,
	LightGizmo,
	LocalSpace,
	WorldSpace,

	SnapRotation,
	SnapScale,
	SnapGrid,
	ShowGizmos,

	FullscreenOnPlay,

	GetMaterial,
	SetMaterial,

	Close,
	Minimize,
	Maximize,
	Windowize,

	Paint,
	Select,
	Fill,
	Swap,
	Remove,

	Warning,

	Volt,
};

enum class EditorMesh
{
	Cube = 0,
	Capsule,
	Cone,
	Cylinder,
	Plane,
	Sphere,
	Arrow,
	Camera
};

class EditorResources
{
public:
	static void Initialize();
	static void Shutdown();

	static IntRef<Volt::RHI::Image> GetAssetIcon(AssetType type);
	static IntRef<Volt::RHI::Image> GetEditorIcon(EditorIcon icon);
	static Ref<Volt::Mesh> GetEditorMesh(EditorMesh mesh);

private:
	static void TryLoadIcon(const Filesystem::Path& path, IntRef<Volt::RHI::Image>* outTexture);
	static Ref<Volt::Mesh> TryLoadMesh(const Filesystem::Path& path);

	inline static std::unordered_map<AssetType, IntRef<Volt::RHI::Image>> m_assetIcons;
	inline static std::unordered_map<EditorIcon, IntRef<Volt::RHI::Image>> m_editorIcons;
	inline static std::unordered_map<EditorMesh, Ref<Volt::Mesh>> m_editorMeshes;

	EditorResources() = delete;
};
