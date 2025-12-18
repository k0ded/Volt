#include "sbpch.h"
#include "Sandbox/Window/ProjectConversionPanel.h"
#include "Sandbox/Utility/EditorUtilities.h"

#include <Volt-Application/UI/UIUtility.h>

#include <Volt-Core/Project/Project.h>
#include <Volt-Core/PluginSystem/PluginRegistry.h>
#include <Volt-Core/Project/ProjectManager.h>

#include <Volt-Renderer/Mesh/Mesh.h>

#include <Volt-Assets/MeshAsset.h>
#include <Volt-Scene/Prefab.h>
#include <Volt-Scene/EntityUtility.h>
#include <Volt-Scene/EntityDescription.h>

#include <Volt-Physics/ColliderComponents.h>
#include <Volt-Physics/RigidbodyComponent.h>
#include <Volt-Physics/CharacterControllerComponent.h>

#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-CoreComponents/LightComponents.h>

#include <SubSystem/SubSystemManager.h>

#include <CoreUtilities/FileSystem.h>
#include <CoreUtilities/FileIO/YAMLFileStreamReader.h>
#include <CoreUtilities/Profiling/Profiling.h>

using namespace Volt;

constexpr const char* MetafileFileExtension = ".vtmeta";

static Map<std::string, AssetType> g_fileExtensionToAssetType
{
	{ ".fbx", AssetTypes::MeshSource },
	{ ".gltf", AssetTypes::MeshSource },
	{ ".glb", AssetTypes::MeshSource },
	{ ".vtmesh", AssetTypes::Mesh },
	{ ".vtnavmesh", AssetTypes::NavMesh },

	{ ".vtsk", AssetTypes::Skeleton },
	{ ".vtanim", AssetTypes::Animation },
	//{ ".vtanimgraph", AssetTypes::AnimationGraph },

	{ ".png", AssetTypes::Texture },
	{ ".jpg", AssetTypes::Texture },
	{ ".jpeg", AssetTypes::Texture },
	{ ".tga", AssetTypes::Texture },
	{ ".ktx", AssetTypes::Texture },
	{ ".dds", AssetTypes::Texture },
	{ ".hdr", AssetTypes::Texture },

	//{ ".vtsdef", AssetTypes::Shader },
	//{ ".hlsl", AssetTypes::ShaderSource },
	//{ ".hlslh", AssetTypes::ShaderSource },
	//{ ".hlsli", AssetTypes::ShaderSource },

	//{ ".vtmgraph", AssetTypes::MaterialGraph },
	{ ".vtmat", AssetTypes::Material },
	{ ".vtpostmat", AssetTypes::PostProcessingMaterial },
	{ ".vtpoststack", AssetTypes::PostProcessingStack },
	{ ".vtphysmat", AssetTypes::PhysicsMaterial },

	{ ".vtscene", AssetTypes::Scene },
	{ ".vtprefab", AssetTypes::Prefab },
	//{ ".vtpp", AssetTypes::ParticlePreset },
	{ ".ttf", AssetTypes::Font },

	//{ ".mp4", AssetTypes::Video },
	//{ ".vtrp", AssetTypes::RenderPipeline },
	//{ ".vtgk", AssetTypes::GraphKey },

	{ ".vtbt", AssetTypes::BehaviorGraph},
	{ ".vtblend", AssetTypes::BlendSpace },

	{ ".vtncon", AssetTypes::NetContract },
};

namespace Wire::ComponentRegistry
{
	enum class PropertyType : uint32_t
	{
		Bool = 0,
		Int = 1,
		UInt = 2,
		Short = 3,
		UShort = 4,
		Char = 5,
		UChar = 6,
		Float = 7,
		Double = 8,
		Vector2 = 9,
		Vector3 = 10,
		Vector4 = 11,
		String = 12,
		Unknown = 13,

		Int64 = 14,
		UInt64 = 15,
		AssetHandle = 16,
		Color3 = 17,
		Color4 = 18,
		Folder = 19,
		Path = 20,
		Vector = 21,
		EntityId = 22,
		GUID = 23,
		Enum = 24,
		Quaternion = 25
	};
}

template<typename T>
void RegisterDeserializationFunction(Map<TypeTraits::TypeIndex, std::function<void(YAMLFileStreamReader&, uint8_t*, const size_t)>>& outTypes)
{
	outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLFileStreamReader& streamReader, uint8_t* data, const size_t offset)
	{
		*reinterpret_cast<T*>(&data[offset]) = streamReader.ReadAtKey("data", T());
	};
}

template<typename T>
void RegisterVectorDeserializationFunction(Map<TypeTraits::TypeIndex, std::function<void(YAMLFileStreamReader&, uint8_t*, const size_t)>>& outTypes)
{
	outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLFileStreamReader& streamReader, uint8_t* data, const size_t offset)
	{
		*reinterpret_cast<T*>(&data[offset]) = streamReader.ReadAtKey("value", T());
	};
}

Map<TypeTraits::TypeIndex, std::function<void(YAMLFileStreamReader&, uint8_t*, const size_t)>> g_deserializationFunctions;
Map<TypeTraits::TypeIndex, std::function<void(YAMLFileStreamReader&, uint8_t*, const size_t)>> g_vectorDeserializationFunctions;

Map<VoltGUID, Map<std::string_view, uint32_t>> g_componentPropertyRemapping;
Map<const IComponentTypeDesc*, std::unordered_set<std::string>> g_missingMembers;

template<typename T>
void AddRemapping(std::string_view oldName, uint32_t identifier)
{
	g_componentPropertyRemapping[GetTypeGUID<T>()][oldName] = identifier;
}

const ComponentMember* TryGetComponentMemberFromName(const IComponentTypeDesc* typeDesc, std::string_view name)
{
	auto componentIt = g_componentPropertyRemapping.find(typeDesc->GetGUID());
	const ComponentMember* componentMember = nullptr;

	// Try to find using remapping
	if (componentIt != g_componentPropertyRemapping.end())
	{
		auto memberIt = componentIt->second.find(name);
		if (memberIt != componentIt->second.end())
		{
			componentMember = typeDesc->FindMemberByIdentifier(memberIt->second);
		}
	}

	// If not found, try finding by label
	if (componentMember == nullptr)
	{
		componentMember = typeDesc->FindMemberByLabel(name);
	}

	// If still null, log a warning
	if (componentMember == nullptr)
	{
		g_missingMembers[typeDesc].insert(std::string(name));
		VT_LOG(Warning, "Unable to find member with old name {} in component {}", name, typeDesc->GetLabel());
	}

	return componentMember;
}

ProjectConversionPanel::ProjectConversionPanel()
	: EditorWindow("ProjectConversionPanel")
{
	{
		RegisterDeserializationFunction<int8_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<uint8_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<int16_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<uint16_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<int32_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<uint32_t>(g_deserializationFunctions);
		RegisterDeserializationFunction<float>(g_deserializationFunctions);
		RegisterDeserializationFunction<double>(g_deserializationFunctions);
		RegisterDeserializationFunction<bool>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::vec2>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::vec3>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::vec4>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::uvec2>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::uvec3>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::uvec4>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::ivec2>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::ivec3>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::ivec4>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::quat>(g_deserializationFunctions);
		RegisterDeserializationFunction<glm::mat4>(g_deserializationFunctions);
		RegisterDeserializationFunction<VoltGUID>(g_deserializationFunctions);
		RegisterDeserializationFunction<std::string>(g_deserializationFunctions);
		RegisterDeserializationFunction<std::filesystem::path>(g_deserializationFunctions);
		RegisterDeserializationFunction<Volt::EntityID>(g_deserializationFunctions);
		RegisterDeserializationFunction<AssetHandle>(g_deserializationFunctions);
	}

	{
		RegisterVectorDeserializationFunction<int8_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<uint8_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<int16_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<uint16_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<int32_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<uint32_t>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<float>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<double>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<bool>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::vec2>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::vec3>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::vec4>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::uvec2>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::uvec3>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::uvec4>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::ivec2>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::ivec3>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::ivec4>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::quat>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<glm::mat4>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<VoltGUID>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<std::string>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<std::filesystem::path>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<Volt::EntityID>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<AssetHandle>(g_vectorDeserializationFunctions);
	}

	// Component remapping
	{
		// TagComponent
		AddRemapping<TagComponent>("tag", 'tag');

		// IDComponent
		AddRemapping<IDComponent>("id", 'id');

		// TransformComponent
		AddRemapping<TransformComponent>("position", 'pos');
		AddRemapping<TransformComponent>("rotation", 'rot');
		AddRemapping<TransformComponent>("scale", 'scal');
		AddRemapping<TransformComponent>("visible", 'vis');
		AddRemapping<TransformComponent>("locked", 'lock');
		AddRemapping<TransformComponent>("movability", 'mvbl');

		// RelationshipComponent
		AddRemapping<RelationshipComponent>("parent", 'par');
		AddRemapping<RelationshipComponent>("children", 'chld');

		// CommonComponent
		AddRemapping<CommonComponent>("timecreatedid", 'time');

		// RigidbodyComponent
		AddRemapping<RigidbodyComponent>("Type", 'bdtp');
		AddRemapping<RigidbodyComponent>("Disable Gravity", 'dsgr');
		AddRemapping<RigidbodyComponent>("layerId", 'lyrd');
		AddRemapping<RigidbodyComponent>("mass", 'mass');
		AddRemapping<RigidbodyComponent>("linearDrag", 'lndr');
		AddRemapping<RigidbodyComponent>("lockFlags", 'lcfl');
		AddRemapping<RigidbodyComponent>("angularDrag", 'andr');
		AddRemapping<RigidbodyComponent>("collisionType", 'colt');
		AddRemapping<RigidbodyComponent>("isKinematic", 'iskn');

		// BoxColliderComponent
		AddRemapping<BoxColliderComponent>("halfSize", 'hasi');
		AddRemapping<BoxColliderComponent>("offset", 'offs');
		AddRemapping<BoxColliderComponent>("isTrigger", 'istr');
		AddRemapping<BoxColliderComponent>("Physics Material", 'mat');

		// SphereColliderComponent
		AddRemapping<SphereColliderComponent>("radius", 'radi');
		AddRemapping<SphereColliderComponent>("offset", 'offs');
		AddRemapping<SphereColliderComponent>("isTrigger", 'istr');
		AddRemapping<SphereColliderComponent>("Physics Material", 'mat');

		// CapsuleColliderComponent
		AddRemapping<CapsuleColliderComponent>("radius", 'radi');
		AddRemapping<CapsuleColliderComponent>("height", 'heig');
		AddRemapping<CapsuleColliderComponent>("offset", 'offs');
		AddRemapping<CapsuleColliderComponent>("isTrigger", 'istr');
		AddRemapping<CapsuleColliderComponent>("Physics Material", 'mat');

		// MeshColliderComponent
		AddRemapping<MeshColliderComponent>("colliderMesh", 'clme');
		AddRemapping<MeshColliderComponent>("isConvex", 'isco');
		AddRemapping<MeshColliderComponent>("isTrigger", 'istr');
		AddRemapping<MeshColliderComponent>("Physics Material", 'mat');
		AddRemapping<MeshColliderComponent>("Sub Mesh Index", 'smi');

		// CharacterControllerComponent
		AddRemapping<CharacterControllerComponent>("climbingMode", 'clim');
		AddRemapping<CharacterControllerComponent>("slopeLimit", 'slli');
		AddRemapping<CharacterControllerComponent>("invisibleWallHeight", 'iwh');
		AddRemapping<CharacterControllerComponent>("maxJumpHeight", 'mjh');
		AddRemapping<CharacterControllerComponent>("contactOffset", 'coff');
		AddRemapping<CharacterControllerComponent>("stepOffset", 'soff');
		AddRemapping<CharacterControllerComponent>("density", 'dens');
		AddRemapping<CharacterControllerComponent>("layer", 'layr');
		AddRemapping<CharacterControllerComponent>("hasGravity", 'hasg');

		// PointLightComponent
		AddRemapping<PointLightComponent>("intensity", 'inte');
		AddRemapping<PointLightComponent>("radius", 'radi');
		AddRemapping<PointLightComponent>("falloff", 'fall');
		AddRemapping<PointLightComponent>("color", 'col');
		AddRemapping<PointLightComponent>("castShadows", 'shdw');

		// SpotLightComponent
		AddRemapping<SpotLightComponent>("intensity", 'inte');
		AddRemapping<SpotLightComponent>("innerAngle", 'angi');
		AddRemapping<SpotLightComponent>("range", 'rang');
		AddRemapping<SpotLightComponent>("color", 'col');
		AddRemapping<SpotLightComponent>("castShadows", 'shdw');
		AddRemapping<SpotLightComponent>("Angle", 'ango');
		AddRemapping<SpotLightComponent>("Angle Attenuation", 'fall');

		// SphereLightComponent
		AddRemapping<SphereLightComponent>("intensity", 'inte');
		AddRemapping<SphereLightComponent>("radius", 'radi');
		AddRemapping<SphereLightComponent>("color", 'col');

		// RectangleLightComponent
		AddRemapping<RectangleLightComponent>("intensity", 'inte');
		AddRemapping<RectangleLightComponent>("color", 'col');
		AddRemapping<RectangleLightComponent>("width", 'wid');
		AddRemapping<RectangleLightComponent>("height", 'heig');

		// DirectionalLightComponent
		AddRemapping<DirectionalLightComponent>("intensity", 'inte');
		AddRemapping<DirectionalLightComponent>("color", 'col');
		AddRemapping<DirectionalLightComponent>("lightSize", 'lisz');
		AddRemapping<DirectionalLightComponent>("sunRadius", 'snrd');
		AddRemapping<DirectionalLightComponent>("softShadows", 'sfsh');
		AddRemapping<DirectionalLightComponent>("castShadows", 'shdw');

		// SkylightComponent
		AddRemapping<SkylightComponent>("intensity", 'inte');
		AddRemapping<SkylightComponent>("lod", 'lod');
		AddRemapping<SkylightComponent>("show", 'show');
		AddRemapping<SkylightComponent>("Environment Map", 'env');


		// AnimationPlayerComponent
		AddRemapping<AnimationPlayerComponent>("skeleton", 'skel');
		AddRemapping<AnimationPlayerComponent>("animationHandle", 'anim');
		AddRemapping<AnimationPlayerComponent>("currentPlayTime", 'play');

		// MeshComponent
		AddRemapping<MeshComponent>("handle", 'hndl');
		AddRemapping<MeshComponent>("materials", 'mats');

		// CameraComponent
		AddRemapping<CameraComponent>("priority", 'prio');
		AddRemapping<CameraComponent>("Field of View", 'fov');
		AddRemapping<CameraComponent>("Near plane", 'nrpl');
		AddRemapping<CameraComponent>("Far plane", 'frpl');

		// TextRendererComponent
		AddRemapping<TextRendererComponent>("text", 'text');
		AddRemapping<TextRendererComponent>("font", 'font');
		AddRemapping<TextRendererComponent>("maxWidth", 'mxwd');
		AddRemapping<TextRendererComponent>("color", 'col');

		// SpriteComponent
		AddRemapping<SpriteComponent>("materialHandle", 'hndl');

		// DecalComponent
		AddRemapping<DecalComponent>("decalMaterial", 'dcl');

		// PrefabComponent
		AddRemapping<PrefabComponent>("PrefabAsset", 'prea');
		AddRemapping<PrefabComponent>("prefabEntity", 'pree');
		AddRemapping<PrefabComponent>("PrefabEntity", 'pree');
		AddRemapping<PrefabComponent>("Version", 'ver');
		AddRemapping<PrefabComponent>("sceneRootEntity", 'sre');
		AddRemapping<PrefabComponent>("componentLocalChanges", 'clc');
	}
}

void ProjectConversionPanel::UpdateMainContent()
{
	if (UI::BeginProperties(""))
	{
		UI::PropertyFile("Project To Convert", m_projectToConvertFilepath, { FileFilter{ "Volt Project", "vtproj" } });
		UI::PropertyDirectory("Target Directory", m_targetDirectory);

		UI::EndProperties();
	}

	if (ImGui::Button("Try Convert Project"))
	{
		if (FileSystem::Exists(m_projectToConvertFilepath) && m_projectToConvertFilepath.extension().string() == ".vtproj")
		{
			TryConvertProject();
		}
	}
}

void ProjectConversionPanel::TryConvertProject()
{
	Project project;
	if (!TryLoadProject(project))
	{
		return;
	}

	m_assetManager = CreateScope<AssetManager>(ProjectManager::GetEngineRootDirectory(), m_targetDirectory, project.assetsDirectoryName);

	const bool hasMetafiles = project.engineVersion.GetMinor() < 5;

	Vector<AssetMetadata> assetMetadata;

	if (hasMetafiles)
	{
		LoadAssetMetadataFromMetaFiles(project, assetMetadata);
	}
	else
	{

	}

	TryConvertAssets(project, assetMetadata);

	PrintMissingMembers();
}

bool ProjectConversionPanel::TryLoadProject(Volt::Project& project)
{
	YAMLFileStreamReader projectFileReader;
	if (!projectFileReader.OpenFile(m_projectToConvertFilepath))
	{
		VT_LOG(Error, "Unable to open project file {}", m_projectToConvertFilepath);
		return false;
	}

	if (!projectFileReader.HasKey("Project"))
	{
		VT_LOG(Error, "Project file {} is invalid!", m_projectToConvertFilepath);
		return false;
	}

	projectFileReader.EnterScope("Project");

	project.engineVersion = projectFileReader.ReadAtKey("EngineVersion", std::string(""));
	project.name = projectFileReader.ReadAtKey("Name", std::string("None"));
	project.companyName = projectFileReader.ReadAtKey("CompanyName", std::string("None"));

	project.assetsDirectoryName = projectFileReader.ReadAtKey("AssetsDirectory", std::string(""));
	if (project.assetsDirectoryName.empty())
	{
		project.assetsDirectoryName = projectFileReader.ReadAtKey("AssetsPath", std::string("Assets"));
	}

	project.audioDirectory = projectFileReader.ReadAtKey("AudioBanksDirectory", std::string(""));
	if (project.audioDirectory.empty())
	{
		project.audioDirectory = projectFileReader.ReadAtKey("AudioBanksPath", std::filesystem::path("Audio/Banks"));
	}

	project.iconFilepath = projectFileReader.ReadAtKey("IconPath", std::filesystem::path(""));
	project.cursorFilepath = projectFileReader.ReadAtKey("CursorPath", std::filesystem::path(""));

	project.startSceneFilepath = projectFileReader.ReadAtKey("StartScenePath", std::filesystem::path(""));
	if (project.startSceneFilepath.empty())
	{
		project.startSceneFilepath = projectFileReader.ReadAtKey("StartScene", std::filesystem::path(""));
	}

	PluginRegistry* pluginRegistry = SubSystemManager::GetSubSystem<PluginRegistry>();

	projectFileReader.ForEach("Plugins", [&]()
	{
		const std::string pluginName = projectFileReader.ReadValue<std::string>();
		const auto& definition = pluginRegistry->GetPluginDefinitionByName(pluginName);
		if (definition.guid != VoltGUID::Null())
		{
			project.pluginDefinitions.emplace_back(definition);
		}
		else
		{
			VT_LOG(Warning, "Plugin with name {} does not exist!", pluginName);
		}
	});

	projectFileReader.ExitScope();

	project.rootDirectory = m_projectToConvertFilepath.parent_path();
	return true;
}

void ProjectConversionPanel::TryConvertAssets(const Volt::Project& project, const ArrayView<Volt::AssetMetadata>& assetMetadata)
{
	Vector<AssetReference<Asset>> assetsToSave;
	Map<AssetHandle, AssetReference<Prefab>> assetHandleToPrefab;

	// Make sure all prefabs are processed first.
	for (const AssetMetadata& metadata : assetMetadata)
	{
		if (metadata.type == AssetTypes::Prefab)
		{
			AssetReference<Prefab> prefab = TryConvertPrefab(project, metadata);
			if (prefab)
			{
				assetsToSave.emplace_back(prefab);
				assetHandleToPrefab[metadata.handle] = prefab;
			}
		}
	}

	for (const AssetMetadata& metadata : assetMetadata)
	{
		if (metadata.type == AssetTypes::Scene)
		{
			Vector<AssetReference<Asset>> assets = TryConvertScene(project, metadata, assetHandleToPrefab);
			assetsToSave.append(assets);
		}
		else if (metadata.type == AssetTypes::Mesh)
		{
			AssetReference<MeshAsset> mesh = TryConvertMesh(project, metadata);
			if (mesh)
			{
				assetsToSave.emplace_back(mesh);
			}
		}
	}

	for (auto& asset : assetsToSave)
	{
		m_assetManager->SaveAsset(asset);
	}
}

Vector<AssetReference<Asset>> ProjectConversionPanel::TryConvertScene(const Volt::Project& project, const Volt::AssetMetadata& metadata, const Map<Volt::AssetHandle, AssetReference<Volt::Prefab>>& prefabs)
{
	const std::filesystem::path absoluteScenePath = project.rootDirectory / metadata.filepath;

	if (!FileSystem::Exists(absoluteScenePath))
	{
		return {};
	}

	YAMLFileStreamReader streamReader;
	if (!streamReader.OpenFile(absoluteScenePath))
	{
		return {};
	}

	if (!streamReader.HasKey("Scene"))
	{
		return {};
	}

	streamReader.EnterScope("Scene");
	std::string sceneName = streamReader.ReadAtKey("name", std::string(""));
	streamReader.ExitScope();

	const std::filesystem::path layersDirectoryPath = absoluteScenePath.parent_path() / "Layers";

	if (!FileSystem::Exists(layersDirectoryPath))
	{
		return {};
	}

	Vector<std::filesystem::path> layerFilepaths;

	for (const auto& it : std::filesystem::directory_iterator(layersDirectoryPath))
	{
		if (!it.is_directory() && it.path().extension().string() == ".vtlayer")
		{
			layerFilepaths.emplace_back(it.path());
		}
	}

	AssetReference<Scene> scene = m_assetManager->CreateAssetAndFileWithAssetHandle<Scene>(metadata.filepath.parent_path(), sceneName, metadata.handle);
	scene.Lock();

	Vector<AssetReference<Asset>> resultAssets;
	resultAssets.emplace_back(scene);

	const std::filesystem::path entitiesTargetDir = metadata.filepath.parent_path() / (metadata.filepath.stem().string() + "_Entities");

	for (const std::filesystem::path& layerFilepath : layerFilepaths)
	{
		YAMLFileStreamReader layerReader;
		if (!layerReader.OpenFile(layerFilepath))
		{
			continue;
		}

		if (!layerReader.HasKey("Layer"))
		{
			continue;
		}

		layerReader.EnterScope("Layer");
		layerReader.ForEach("Entities", [&]() 
		{
			VT_PROFILE_SCOPE("ConvertEntity");

			layerReader.EnterScope("Entity");

			// Entity id is serialized as a uint32_t.
			EntityID entityId = layerReader.ReadAtKey("id", 0u);

			const std::string entityDescName = std::to_string(entityId);
			AssetReference<EntityDesc> entityDescription = m_assetManager->CreateAsset<EntityDesc>(entityDescName, entityId, scene->GetAssetHandle());
			resultAssets.emplace_back(entityDescription);

			Entity newEntity = scene->AddEntityToScene(entityDescription);
			entityDescription.Lock();
			m_assetManager->CreateFileForAsset(entityDescription->GetAssetHandle(), entitiesTargetDir / (entityDescName + ".vtasset"));
			entityDescription.Unlock();

			layerReader.ForEach("components", [&]() 
			{
				const VoltGUID componentGUID = layerReader.ReadAtKey("guid", VoltGUID::Null());
				if (componentGUID == VoltGUID::Null())
				{
					return;
				}

				const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(componentGUID);
				if (!typeDesc)
				{
					return;
				}

				switch (typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						if (!ComponentRegistry::Helpers::HasComponentWithGUID(componentGUID, scene->GetEntityScene().GetRegistry(), newEntity.GetHandle()))
						{
							ComponentRegistry::Helpers::AddComponentWithGUID(componentGUID, scene->GetEntityScene().GetRegistry(), newEntity.GetHandle());
						}

						uint8_t* componentData = reinterpret_cast<uint8_t*>(ComponentRegistry::Helpers::GetComponentWithGUID(componentGUID, scene->GetEntityScene().GetRegistry(), newEntity.GetHandle()));
						const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);

						layerReader.ForEach("properties", [&]() 
						{
							Wire::ComponentRegistry::PropertyType type = static_cast<Wire::ComponentRegistry::PropertyType>(layerReader.ReadAtKey("type", static_cast<uint32_t>(Wire::ComponentRegistry::PropertyType::Unknown)));
							Wire::ComponentRegistry::PropertyType vectorType = static_cast<Wire::ComponentRegistry::PropertyType>(layerReader.ReadAtKey("vectorType", static_cast<uint32_t>(Wire::ComponentRegistry::PropertyType::Unknown)));
							std::string name = layerReader.ReadAtKey("name", std::string(""));
						
							VT_UNUSED(vectorType);

							if (type == Wire::ComponentRegistry::PropertyType::Unknown)
							{
								return;
							}

							const ComponentMember* componentMember = TryGetComponentMemberFromName(componentDesc, name);
							if (!componentMember)
							{
								return;
							}

							// Non array types
							if (type != Wire::ComponentRegistry::PropertyType::Vector && componentMember->typeDesc == nullptr)
							{
								if (g_deserializationFunctions.contains(componentMember->typeIndex))
								{
									g_deserializationFunctions.at(componentMember->typeIndex)(layerReader, componentData, componentMember->offset);
								}
							}
							// The value is an array, and needs to be deserialized as such.
							else if (componentMember->typeDesc && componentMember->typeDesc->GetValueType() == ValueType::Array)
							{
								const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(componentMember->typeDesc);
								const auto& arrayTypeIndex = arrayTypeDesc->GetElementTypeIndex();

								void* arrayPtr = &componentData[componentMember->offset];

								layerReader.ForEach("data", [&]() 
								{
									void* tempDataStorage = nullptr;
									arrayTypeDesc->DefaultConstructElement(tempDataStorage);

									uint8_t* tempBytePtr = reinterpret_cast<uint8_t*>(tempDataStorage);

									if (g_vectorDeserializationFunctions.contains(arrayTypeIndex))
									{
										g_vectorDeserializationFunctions.at(arrayTypeIndex)(layerReader, tempBytePtr, 0);
									}

									arrayTypeDesc->PushBack(arrayPtr, tempDataStorage);
									arrayTypeDesc->DestroyElement(tempDataStorage);
								});
							}
						});

						break;
					}
				}
			});

			layerReader.ExitScope();
		});
		layerReader.ExitScope();

		// Fixup prefabs
		{
			auto view = scene->GetEntityScene().GetRegistry().view<PrefabComponent>();

			for (const auto& entId : view)
			{
				Entity entity = scene->GetEntityFromHandle(entId);
				if (entity)
				{
					PrefabComponent& prefabComponent = entity.GetComponent<PrefabComponent>();

					// Find scene root id
					Entity parentEntity = entity.GetParent();
					Entity lastTopEntity = entity;
					while (parentEntity != Entity::Null())
					{
						if (parentEntity.HasComponent<PrefabComponent>())
						{
							// It's the same prefab, let's look one step further
							if (parentEntity.GetComponent<PrefabComponent>().prefabAsset == prefabComponent.prefabAsset)
							{
								lastTopEntity = parentEntity;
								parentEntity = parentEntity.GetParent();
							}
							else
							{
								break;
							}
						}
						else
						{
							break;
						}
					}

					prefabComponent.sceneRootEntity = lastTopEntity.GetID();
					VT_ENSURE(prefabComponent.sceneRootEntity != EntityID::Null());

					// Update entity
					auto prefabIt = prefabs.find(prefabComponent.prefabAsset);
					if (prefabIt != prefabs.end())
					{
						AssetReference<Prefab> prefab = prefabIt->second;
						ScopedAssetReferenceLock prefabLock{ prefab };

						prefab->CopyPrefabEntity(entity, prefabComponent.prefabEntity,
							Volt::CreateSkipComponentOnCopySet<RelationshipComponent, TransformComponent, IDComponent, PrefabComponent>());
					}
				}
			}
		}
	}

	scene.Unlock();

	VT_LOG(Trace, "Converted Scene with name {}", sceneName);
	return resultAssets;
}

AssetReference<Volt::MeshAsset> ProjectConversionPanel::TryConvertMesh(const Volt::Project& project, const Volt::AssetMetadata& metadata)
{
	struct LegacyVertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texCoords;
		glm::uvec4 influences;
		glm::vec4 weights;
	};

	const std::filesystem::path absoluteMeshPath = project.rootDirectory / metadata.filepath;

	if (!FileSystem::Exists(absoluteMeshPath))
	{
		return nullptr;
	}

	Buffer dataBuffer = Buffer::ReadFromFile(absoluteMeshPath);

	const std::string meshName = absoluteMeshPath.stem().string();
	AssetReference<MeshAsset> newMesh = m_assetManager->CreateAssetAndFileWithAssetHandle<MeshAsset>(metadata.filepath.parent_path(), metadata.filepath.stem().string(), metadata.handle);

	{
		ScopedAssetReferenceLock meshLock{ newMesh };

		size_t offset = 0;

		const uint32_t numSubMeshes = *dataBuffer.As<uint32_t>(offset);
		offset += sizeof(uint32_t);

		const AssetHandle materialHandle = *dataBuffer.As<AssetHandle>(offset);
		offset += sizeof(AssetHandle);

		const uint32_t numVertices = *dataBuffer.As<uint32_t>(offset);
		offset += sizeof(uint32_t);

		Vector<LegacyVertex> legacyVertices;
		legacyVertices.resize_uninitialized(numVertices);

		memcpy_s(legacyVertices.data(), legacyVertices.byte_size(), dataBuffer.As<LegacyVertex>(offset), sizeof(LegacyVertex)* numVertices);
		offset += sizeof(LegacyVertex) * numVertices;

		const uint32_t numIndices = *dataBuffer.As<uint32_t>(offset);
		offset += sizeof(uint32_t);

		Vector<uint32_t> indices;
		indices.resize_uninitialized(numIndices);

		memcpy_s(indices.data(), indices.byte_size(), dataBuffer.As<uint32_t>(offset), sizeof(uint32_t)* numIndices);
		offset += sizeof(uint32_t) * numIndices;

		// Skip bounding sphere
		offset += sizeof(glm::vec3) + sizeof(float);

		const uint32_t numSubMeshNames = *dataBuffer.As<uint32_t>(offset);
		offset += sizeof(uint32_t);

		Vector<std::string> names;
		names.reserve(numSubMeshNames);

		for (uint32_t i = 0; i < numSubMeshNames; ++i)
		{
			const uint32_t nameSize = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			const char* nameData = dataBuffer.As<const char>(offset);
			names.emplace_back(nameData);

			offset += nameSize;
		}

		MeshInitializer meshInitializer;

		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			SubMesh newSubMesh;

			// Note: All sub meshes get the same material as we no longer have sub materials.
			//newSubMesh.materialIndex = *dataBuffer.As<uint32_t>(offset);
			newSubMesh.materialIndex = 0;
			offset += sizeof(uint32_t);

			newSubMesh.vertexCount = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.indexCount = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.vertexStartOffset = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.indexStartOffset = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			glm::mat4 transform = *dataBuffer.As<glm::mat4>(offset);

			glm::quat r;
			glm::vec3 t, s;
			Math::Decompose(transform, t, r, s);

			newSubMesh.transform.position = t;
			newSubMesh.transform.rotation = r;
			newSubMesh.transform.scale = s;

			offset += sizeof(glm::mat4);

			if (i < static_cast<uint32_t>(names.size()))
			{
				newSubMesh.name = names.at(i);
			}

			meshInitializer.AddSubMesh(newSubMesh);
		}

		meshInitializer.SetIndices(indices);

		// Convert vertices
		{
			VertexContainer vertexContainer;

			for (const LegacyVertex& vertex : legacyVertices)
			{
				const VertexMaterialData materialData = VertexMaterialData::Pack(vertex.normal, { vertex.tangent, 0.f }, vertex.texCoords);
				const VertexAnimationData animationData = { vertex.influences, vertex.weights };

				vertexContainer.Add(vertex.position, materialData, animationData);
			}

			meshInitializer.AddVertices(vertexContainer);
		}

		newMesh->Initialize(meshInitializer, { materialHandle });

		VT_LOG(Trace, "Converted Mesh with name {}", meshName);
	}

	return newMesh;
}

AssetReference<Prefab> ProjectConversionPanel::TryConvertPrefab(const Volt::Project& project, const Volt::AssetMetadata& metadata)
{
	const std::filesystem::path absolutePrefabPath = project.rootDirectory / metadata.filepath;
	
	if (!FileSystem::Exists(absolutePrefabPath))
	{
		return nullptr;
	}

	YAMLFileStreamReader streamReader;
	if (!streamReader.OpenFile(absolutePrefabPath))
	{
		return nullptr;
	}

	if (!streamReader.HasKey("Prefab"))
	{
		return nullptr;
	}

	AssetReference<Scene> prefabScene = m_assetManager->CreateMemoryAsset<Scene>("");
	ScopedAssetReferenceLock prefabLock{ prefabScene };

	EntityID rootEntityId = EntityID::Null();

	streamReader.EnterScope("Prefab");
	const uint32_t version = streamReader.ReadAtKey("version", 0u);

	streamReader.ForEach("entities", [&]() 
	{
		EntityID entityId = streamReader.ReadAtKey("id", EntityID::Null());
		if (entityId == EntityID::Null())
		{
			return;
		}

		Entity prefabEntity = prefabScene->CreateEntityWithID(entityId);
		
		streamReader.ForEach("components", [&]() 
		{
			const VoltGUID componentGUID = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (componentGUID == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(componentGUID);
			if (!typeDesc)
			{
				return;
			}

			switch (typeDesc->GetValueType())
			{
				case ValueType::Component:
				{
					if (!ComponentRegistry::Helpers::HasComponentWithGUID(componentGUID, prefabScene->GetEntityScene().GetRegistry(), prefabEntity.GetHandle()))
					{
						ComponentRegistry::Helpers::AddComponentWithGUID(componentGUID, prefabScene->GetEntityScene().GetRegistry(), prefabEntity.GetHandle());
					}

					uint8_t* componentData = reinterpret_cast<uint8_t*>(ComponentRegistry::Helpers::GetComponentWithGUID(componentGUID, prefabScene->GetEntityScene().GetRegistry(), prefabEntity.GetHandle()));
					const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);

					streamReader.ForEach("properties", [&]()
					{
						Wire::ComponentRegistry::PropertyType type = static_cast<Wire::ComponentRegistry::PropertyType>(streamReader.ReadAtKey("type", static_cast<uint32_t>(Wire::ComponentRegistry::PropertyType::Unknown)));
						Wire::ComponentRegistry::PropertyType vectorType = static_cast<Wire::ComponentRegistry::PropertyType>(streamReader.ReadAtKey("vectorType", static_cast<uint32_t>(Wire::ComponentRegistry::PropertyType::Unknown)));
						std::string name = streamReader.ReadAtKey("name", std::string(""));

						VT_UNUSED(vectorType);

						if (type == Wire::ComponentRegistry::PropertyType::Unknown)
						{
							return;
						}

						const ComponentMember* componentMember = TryGetComponentMemberFromName(componentDesc, name);
						if (!componentMember)
						{
							return;
						}

						// Non array types
						if (type != Wire::ComponentRegistry::PropertyType::Vector && componentMember->typeDesc == nullptr)
						{
							if (g_deserializationFunctions.contains(componentMember->typeIndex))
							{
								g_deserializationFunctions.at(componentMember->typeIndex)(streamReader, componentData, componentMember->offset);
							}
						}
						// The value is an array, and needs to be deserialized as such.
						else if (componentMember->typeDesc && componentMember->typeDesc->GetValueType() == ValueType::Array)
						{
							const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(componentMember->typeDesc);
							const auto& arrayTypeIndex = arrayTypeDesc->GetElementTypeIndex();

							void* arrayPtr = &componentData[componentMember->offset];

							streamReader.ForEach("data", [&]()
							{
								void* tempDataStorage = nullptr;
								arrayTypeDesc->DefaultConstructElement(tempDataStorage);

								uint8_t* tempBytePtr = reinterpret_cast<uint8_t*>(tempDataStorage);

								if (g_vectorDeserializationFunctions.contains(arrayTypeIndex))
								{
									g_vectorDeserializationFunctions.at(arrayTypeIndex)(streamReader, tempBytePtr, 0);
								}

								arrayTypeDesc->PushBack(arrayPtr, tempDataStorage);
								arrayTypeDesc->DestroyElement(tempDataStorage);
							});
						}
					});

					break;
				}
			}
		});
	
		// Verify
		if (prefabEntity.HasComponent<PrefabComponent>())
		{
			const PrefabComponent& prefabComponent = prefabEntity.GetComponent<PrefabComponent>();
			VT_ENSURE(prefabComponent.prefabEntity != EntityID::Null());
			VT_ENSURE(prefabComponent.prefabAsset != Asset::Null());
		}

		if (rootEntityId == EntityID::Null())
		{
			if (prefabEntity.GetParent() == Entity::Null())
			{
				rootEntityId = prefabEntity.GetID();
			}
		}
	});

	streamReader.ExitScope();

	VT_ENSURE(rootEntityId != EntityID::Null());

	AssetReference<Prefab> prefab = m_assetManager->CreateAssetAndFileWithAssetHandle<Prefab>(metadata.filepath.parent_path(), metadata.filepath.stem().string(), metadata.handle, prefabScene, rootEntityId, version);

	VT_LOG(Trace, "Converted Prefab with name {}", metadata.filepath.stem().string());
	return prefab;
}

void ProjectConversionPanel::PrintMissingMembers()
{
	for (const auto& [typeDesc, missingMembers] : g_missingMembers)
	{
		VT_LOG(Warning, "Missing members in \"{}\":", typeDesc->GetLabel());

		for (const auto& memberName : missingMembers)
		{
			VT_LOG(Warning, "	- {}", memberName);
		}
	}
}

void ProjectConversionPanel::LoadAssetMetadataFromMetaFiles(const Volt::Project& project, Vector<Volt::AssetMetadata>& outMetadata)
{
	const std::filesystem::path assetsDirectoryPath = project.rootDirectory / project.assetsDirectoryName;

	if (FileSystem::Exists(assetsDirectoryPath))
	{
		for (auto& pathEntry : std::filesystem::recursive_directory_iterator(assetsDirectoryPath))
		{
			if (pathEntry.path().extension() == MetafileFileExtension)
			{
				AssetMetadata& newMetadata = outMetadata.emplace_back();
				
				YAMLFileStreamReader metadataReader;
				if (!metadataReader.OpenFile(pathEntry.path()))
				{
					continue;
				}

				newMetadata.handle = metadataReader.ReadAtKey("Handle", AssetHandle(0));
				newMetadata.filepath = metadataReader.ReadAtKey("Path", std::filesystem::path(""));
				newMetadata.type = AssetTypes::None;

				const std::filesystem::path absoluteAssetPath = project.rootDirectory / newMetadata.filepath;
				if (FileSystem::Exists(absoluteAssetPath))
				{
					const std::string fileExtension = absoluteAssetPath.extension().string();
					if (g_fileExtensionToAssetType.contains(fileExtension))
					{
						newMetadata.type = g_fileExtensionToAssetType.at(fileExtension);
					}
				}
			}
		}
	}
}
