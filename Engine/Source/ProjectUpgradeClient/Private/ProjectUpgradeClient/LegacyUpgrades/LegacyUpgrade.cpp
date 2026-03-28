#include "ProjectUpgradeClient/LegacyUpgrades/LegacyUpgrade.h"

#include "ProjectUpgradeClient/Common/CommonSerializeFuncs.h"
#include "ProjectUpgradeClient/Common/YAMLFileStreamWriter.h"
#include "ProjectUpgradeClient/Common/YAMLFileStreamReader.h"
#include "ProjectUpgradeClient/Common/YAMLMemoryStreamReader.h"

#include <Volt-Application/UI/UIUtility.h>

#include <CoreModule/Project/Project.h>
#include <CoreModule/PluginSystem/PluginRegistry.h>
#include <CoreModule/Project/ProjectManager.h>

#include <Volt-Renderer/Texture/EnvironmentTexture.h>
#include <Volt-Renderer/Mesh/Mesh.h>
#include <Volt-Renderer/Renderer.h>

#include <Volt-Assets/MeshAsset.h>
#include <Volt-Assets/MaterialAsset.h>
#include <Volt-Assets/SourceAssetImporters/ImportConfigs.h>

#include <Volt-Scene/Prefab.h>
#include <Volt-Scene/EntityUtility.h>
#include <Volt-Scene/EntityDescription.h>
#include <Volt-Scene/Components/CoreComponents.h>

#include <Volt-Physics/ColliderComponents.h>
#include <Volt-Physics/RigidbodyComponent.h>
#include <Volt-Physics/CharacterControllerComponent.h>

#include <Volt-CoreComponents/RenderingComponents.h>
#include <Volt-CoreComponents/LightComponents.h>

#include <Volt-MaterialGraph/MaterialGraph.h>
#include <Volt-MaterialGraph/Nodes/PBROutputNode.h>
#include <Volt-MaterialGraph/Nodes/ConstantNodes.h>
#include <Volt-MaterialGraph/Nodes/Normal/NormalStrengthNode.h>
#include <Volt-MaterialGraph/Nodes/MathNodes.h>
#include <Volt-MaterialGraph/Nodes/Texture/SampleTextureNode.h>
#include <Volt-MaterialGraph/Nodes/Normal/DeriveNormalZNode.h>
#include <Volt-MaterialGraph/Nodes/Normal/NormalStrengthNode.h>
#include <Volt-MaterialGraph/Nodes/ConversionNodes.h>

#include <FileSystemModule/Filesystem.h>
#include <FileSystemModule/Iterators/DirectoryIterator.h>
#include <FileSystemModule/Iterators/RecursiveDirectoryIterator.h>

#include <AssetSystem/SourceAssetManager.h>

#include <Mosaic/MosaicGraphBuilder.h>

#include <SubSystem/SubSystemManager.h>

#include <EntitySystem/Scripting/CommonComponent.h>

#include <CoreUtilities/Profiling/Profiling.h>

using namespace Volt;

constexpr const char* MetafileFileExtension = ".vtmeta";

static Map<String, AssetType> g_fileExtensionToAssetType
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

Map<VoltGUID, Map<StringView, uint32_t>> g_componentPropertyRemapping;
Map<const IComponentTypeDesc*, std::unordered_set<String>> g_missingMembers;

template<typename T>
void AddRemapping(StringView oldName, uint32_t identifier)
{
	g_componentPropertyRemapping[GetTypeGUID<T>()][oldName] = identifier;
}

const ComponentMember* TryGetComponentMemberFromName(const IComponentTypeDesc* typeDesc, StringView name)
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
		g_missingMembers[typeDesc].insert(String(name));
		VT_LOG(Warning, "Unable to find member with old name {} in component {}", name, typeDesc->GetLabel());
	}

	return componentMember;
}

LegacyProjectUpgrade::LegacyProjectUpgrade(const Project& inProject)
	: Upgrade(inProject)
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
		RegisterDeserializationFunction<String>(g_deserializationFunctions);
		RegisterDeserializationFunction<Filesystem::Path>(g_deserializationFunctions);
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
		RegisterVectorDeserializationFunction<String>(g_vectorDeserializationFunctions);
		RegisterVectorDeserializationFunction<Filesystem::Path>(g_vectorDeserializationFunctions);
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

#if 0
void LegacyProjectUpgrade::UpdateMainContent()
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
#endif

void LegacyProjectUpgrade::TryConvertProject(const Filesystem::Path& projectFilepath, const Filesystem::Path& targetDirectory)
{
	m_projectToConvertFilepath = projectFilepath;
	m_targetDirectory = targetDirectory;
}

bool LegacyProjectUpgrade::ProcessUpgrade()
{
	Project project;
	if (!TryLoadProject(project))
	{
		return true;
	}

	g_assetManager = std::move(g_assetManager);
	g_assetManager = CreateUnique<AssetManager>(ProjectManager::GetEngineRootDirectory(), m_targetDirectory, project.assetsDirectoryName);

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

	return true;
}

size_t LegacyProjectUpgrade::GetNumTotalActions()
{
	return 1;
}

size_t LegacyProjectUpgrade::GetNumActionsCompleted()
{
	return 1;
}

String LegacyProjectUpgrade::GetCurrentActionText()
{
	return "Upgrading";
}

bool LegacyProjectUpgrade::TryLoadProject(Volt::Project& project)
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

	project.engineVersion = projectFileReader.ReadAtKey("EngineVersion", String(""));
	project.name = projectFileReader.ReadAtKey("Name", String("None"));
	project.companyName = projectFileReader.ReadAtKey("CompanyName", String("None"));

	project.assetsDirectoryName = projectFileReader.ReadAtKey("AssetsDirectory", String(""));
	if (project.assetsDirectoryName.empty())
	{
		project.assetsDirectoryName = projectFileReader.ReadAtKey("AssetsPath", String("Assets"));
	}

	project.audioDirectory = projectFileReader.ReadAtKey("AudioBanksDirectory", String(""));
	if (project.audioDirectory.IsEmpty())
	{
		project.audioDirectory = projectFileReader.ReadAtKey("AudioBanksPath", Filesystem::Path("Audio/Banks"));
	}

	project.iconFilepath = projectFileReader.ReadAtKey("IconPath", Filesystem::Path(""));
	project.cursorFilepath = projectFileReader.ReadAtKey("CursorPath", Filesystem::Path(""));

	project.startSceneFilepath = projectFileReader.ReadAtKey("StartScenePath", Filesystem::Path(""));
	if (project.startSceneFilepath.IsEmpty())
	{
		project.startSceneFilepath = projectFileReader.ReadAtKey("StartScene", Filesystem::Path(""));
	}

	PluginRegistry* pluginRegistry = SubSystemManager::GetSubSystem<PluginRegistry>();

	projectFileReader.ForEach("Plugins", [&]()
	{
		const String pluginName = projectFileReader.ReadValue<String>();
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

	project.rootDirectory = m_projectToConvertFilepath.ParentPath();
	return true;
}

void LegacyProjectUpgrade::TryConvertAssets(const Volt::Project& project, const ArrayView<Volt::AssetMetadata>& assetMetadata)
{
	Vector<AssetReference<Asset>> assetsToSave;
	Map<AssetHandle, AssetReference<Prefab>> assetHandleToPrefab;
	MaterialsMap materialsMap;

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
		else if (metadata.type == AssetTypes::Material)
		{
			if (!materialsMap.contains(metadata.handle))
			{
				Vector<AssetReference<Asset>> assets = CreateMaterials(project, metadata, materialsMap);
				assetsToSave.append(assets);
			}
		}
		else if (metadata.type == AssetTypes::Mesh)
		{
			Vector<AssetReference<Asset>> assets = TryConvertMesh(project, metadata, assetMetadata, materialsMap);
			assetsToSave.append(assets);
		}
		else if (metadata.type == AssetTypes::Texture)
		{
			assetsToSave.emplace_back(TryConvertTexture(project, metadata));
		}
	}

	// Convert scenes last.
	for (const AssetMetadata& metadata : assetMetadata)
	{
		if (metadata.type == AssetTypes::Scene)
		{
			Vector<AssetReference<Asset>> assets = TryConvertScene(project, metadata, assetHandleToPrefab, materialsMap);
			assetsToSave.append(assets);
		}
	}

	for (auto& asset : assetsToSave)
	{
		// Default to true to skip imported assets.
		bool isMemoryAsset = true;
		{
			ReadOnlyAssetMetadata metadata = g_assetManager->GetReadOnlyAssetMetadata(asset->GetAssetHandle());
			if (metadata.IsValid())
			{
				isMemoryAsset = metadata->IsMemoryAsset();
			}
		}
		
		if (!isMemoryAsset)
		{
			g_assetManager->SaveAsset(asset);
		}
	}
}

Vector<AssetReference<Asset>> LegacyProjectUpgrade::TryConvertScene(const Volt::Project& project, const Volt::AssetMetadata& metadata, const Map<Volt::AssetHandle, AssetReference<Volt::Prefab>>& prefabs, const MaterialsMap& materialsMap)
{
	const Filesystem::Path absoluteScenePath = project.rootDirectory / metadata.filepath;

	if (!Filesystem::Exists(absoluteScenePath))
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
	String sceneName = streamReader.ReadAtKey("name", String(""));
	streamReader.ExitScope();

	const Filesystem::Path layersDirectoryPath = absoluteScenePath.ParentPath() / "Layers";

	if (!Filesystem::Exists(layersDirectoryPath))
	{
		return {};
	}

	Vector<Filesystem::Path> layerFilepaths;

	for (const auto& it : Filesystem::DirectoryIterator(layersDirectoryPath))
	{
		if (!it.isDirectory && it.path.Extension() == L".vtlayer")
		{
			layerFilepaths.emplace_back(it.path);
		}
	}

	AssetReference<Scene> scene = g_assetManager->CreateAssetAndFileWithAssetHandle<Scene>(metadata.filepath.ParentPath(), sceneName, metadata.handle);

	Vector<AssetReference<Asset>> resultAssets;
	resultAssets.emplace_back(scene);

	const Filesystem::Path entitiesTargetDir = metadata.filepath.ParentPath() / (metadata.filepath.Stem().ToString() + "_Entities");

	Vector<AssetReference<EntityDesc>> entityDescs;

	for (const Filesystem::Path& layerFilepath : layerFilepaths)
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

			const String entityDescName = FormatString("{}", entityId);

			Entity newEntity = scene->CreateEntityWithID(entityId);
			AssetHandle entityDescHandle = scene->GetEntityDescHandleFromEntityID(entityId);
			g_assetManager->CreateFileForAsset(entityDescHandle, entitiesTargetDir / (entityDescName + ".vtasset"));

			entityDescs.emplace_back(g_assetManager->GetAssetImmediately<EntityDesc>(entityDescHandle));

			layerReader.ForEach("components", [&]() 
			{
				const VoltGUID componentGUID = layerReader.ReadAtKey("guid", VoltGUID::Null());
				if (componentGUID == VoltGUID::Null())
				{
					return;
				}

				const ICommonTypeDesc* typeDesc = Volt::ComponentRegistry::Get().GetTypeDescFromGUID(componentGUID);
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
							String name = layerReader.ReadAtKey("name", String(""));
						
							VT_UNUSED(vectorType);

							if (type == Wire::ComponentRegistry::PropertyType::Unknown)
							{
								return;
							}

							// Special handling for MeshComponent, we wan't to expand the single material, into the
							// multiple materials the previous sub materials represents
							if (componentGUID == Volt::GetTypeGUID<MeshComponent>() && name == "material")
							{
								AssetHandle materialHandle = layerReader.ReadAtKey("data", AssetHandle(0));
								if (materialHandle != Asset::Null())
								{
									if (materialsMap.contains(materialHandle))
									{
										MeshComponent& meshComponent = newEntity.GetComponent<MeshComponent>();
										meshComponent.materials = materialsMap.at(materialHandle).subMaterials;
									}
								}
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

						prefab->CopyPrefabEntity(entity, prefabComponent.prefabEntity,
							Volt::CreateSkipComponentOnCopySet<RelationshipComponent, TransformComponent, IDComponent, PrefabComponent>());
					}
				}
			}
		}

		// Fixup MeshComponents
		{
			auto view = scene->GetEntityScene().GetRegistry().view<MeshComponent>();

			for (const auto& entId : view)
			{
				Entity entity = scene->GetEntityFromHandle(entId);
				if (entity)
				{
					MeshComponent& meshComponent = entity.GetComponent<MeshComponent>();

					if (meshComponent.materials.empty())
					{
						if (g_assetManager->IsValidAssetHandle(meshComponent.handle))
						{
							AssetReference<MeshAsset> mesh = g_assetManager->GetAssetImmediately<MeshAsset>(meshComponent.handle);
							meshComponent.materials = mesh->GetMaterials();
						}
					}
				}
			}
		}

		for (AssetReference<EntityDesc>& entityDesc : entityDescs)
		{
			bool succeded = entityDesc->UpdateComponentData();
			VT_ENSURE(succeded);

			resultAssets.emplace_back(entityDesc);
		}
	}

	VT_LOG(Trace, "Converted Scene with name {}", sceneName);
	return resultAssets;
}

Vector<AssetReference<Volt::Asset>> LegacyProjectUpgrade::TryConvertMesh(const Volt::Project& project, const Volt::AssetMetadata& metadata, const ArrayView<Volt::AssetMetadata>& assetMetadatas, MaterialsMap& materialsMap)
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

	const Filesystem::Path absoluteMeshPath = project.rootDirectory / metadata.filepath;

	if (!Filesystem::Exists(absoluteMeshPath))
	{
		return {};
	}

	Vector<AssetReference<Volt::Asset>> assets;

	DataBuffer dataBuffer;
	{
		std::filesystem::path tempPath(absoluteMeshPath.ToWString().begin(), absoluteMeshPath.ToWString().end());
		std::ifstream file(tempPath, std::ios::in | std::ios::binary);
		if (!file.is_open())
		{
			return {};
		}

		Vector<uint8_t> totalData;
		const size_t srcSize = file.seekg(0, std::ios::end).tellg();
		totalData.resize(srcSize);
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(totalData.data()), totalData.size());
		file.close();

		dataBuffer.Copy(totalData.data(), totalData.size());
	}

	const String meshName = absoluteMeshPath.Stem().ToString();
	AssetReference<MeshAsset> newMesh = g_assetManager->CreateAssetAndFileWithAssetHandle<MeshAsset>(metadata.filepath.ParentPath(), metadata.filepath.Stem().ToString(), metadata.handle);
	assets.emplace_back(newMesh);

	{
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

		Vector<String> names;
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

		uint32_t requiredMaterialCount = 0;

		for (uint32_t i = 0; i < numSubMeshes; ++i)
		{
			SubMesh newSubMesh;

			// Note: All sub meshes get the same material as we no longer have sub materials.
			newSubMesh.materialIndex = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			requiredMaterialCount = glm::max(requiredMaterialCount, newSubMesh.materialIndex + 1);

			newSubMesh.vertexCount = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.indexCount = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.vertexStartOffset = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			newSubMesh.indexStartOffset = *dataBuffer.As<uint32_t>(offset);
			offset += sizeof(uint32_t);

			glm::mat4 transform = *dataBuffer.As<glm::mat4>(offset);

			for (uint32_t vertexIndex = newSubMesh.vertexStartOffset; vertexIndex < newSubMesh.vertexCount; ++vertexIndex)
			{
				legacyVertices[vertexIndex].position = glm::vec3(transform * glm::vec4(legacyVertices[vertexIndex].position, 1.f));
				legacyVertices[vertexIndex].normal = glm::mat3(transform) * legacyVertices[vertexIndex].normal;
				legacyVertices[vertexIndex].tangent = glm::mat3(transform) * legacyVertices[vertexIndex].tangent;
			}

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
				const VertexMaterialData materialData = VertexMaterialData::Pack(vertex.normal, { vertex.tangent, 1.f }, vertex.texCoords);
				const VertexAnimationData animationData = { vertex.influences, vertex.weights };

				vertexContainer.Add(vertex.position, materialData, animationData);
			}

			meshInitializer.AddVertices(vertexContainer);
		}

		if (!materialsMap.contains(materialHandle))
		{
			for (const Volt::AssetMetadata& matMetadata : assetMetadatas)
			{
				if (matMetadata.handle == materialHandle)
				{
					Vector<AssetReference<Volt::Asset>> createdMaterials = CreateMaterials(project, matMetadata, materialsMap);
					assets.append(createdMaterials);
					break;
				}
			}
		}

		Vector<AssetHandle> materials;
		if (materialsMap.contains(materialHandle))
		{
			materials = materialsMap.at(materialHandle).subMaterials;
		
			if (materials.size() < requiredMaterialCount)
			{
				const size_t prevSize = materials.size();
				materials.resize(requiredMaterialCount);

				for (size_t i = prevSize; i < materials.size(); ++i)
				{
					materials[i] = materials.at(materials.size() - 1);
				}
			}
		}
		else
		{
			// Material doesn't exist. Create fallback
			for (uint32_t i = 0; i < requiredMaterialCount; ++i)
			{
				String materialName = FormatString("{}_Mat_{}", meshName, i);
				AssetReference<Asset> newMaterial = g_assetManager->CreateAssetAndFile<MaterialAsset>(metadata.filepath.ParentPath(), materialName);
				
				materials.emplace_back(newMaterial->GetAssetHandle());
				assets.emplace_back(newMaterial);
			}
		}

		newMesh->Initialize(meshInitializer, materials);

		VT_LOG(Trace, "Converted Mesh with name {}", meshName);
	}

	return assets;
}

AssetReference<Prefab> LegacyProjectUpgrade::TryConvertPrefab(const Volt::Project& project, const Volt::AssetMetadata& metadata)
{
	const Filesystem::Path absolutePrefabPath = project.rootDirectory / metadata.filepath;
	
	if (!Filesystem::Exists(absolutePrefabPath))
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

	AssetReference<Scene> prefabScene = g_assetManager->CreateMemoryAsset<Scene>("");

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

			const ICommonTypeDesc* typeDesc = Volt::ComponentRegistry::Get().GetTypeDescFromGUID(componentGUID);
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
						String name = streamReader.ReadAtKey("name", String(""));

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
			VT_MAYBE_UNUSED const PrefabComponent& prefabComponent = prefabEntity.GetComponent<PrefabComponent>();
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

	AssetReference<Prefab> prefab = g_assetManager->CreateAssetAndFileWithAssetHandle<Prefab>(metadata.filepath.ParentPath(), metadata.filepath.Stem().ToString(), metadata.handle, prefabScene, rootEntityId, version);

	VT_LOG(Trace, "Converted Prefab with name {}", metadata.filepath.Stem().ToString());
	return prefab;
}

void LegacyProjectUpgrade::PrintMissingMembers()
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

void LegacyProjectUpgrade::LoadAssetMetadataFromMetaFiles(const Volt::Project& project, Vector<Volt::AssetMetadata>& outMetadata)
{
	const Filesystem::Path assetsDirectoryPath = project.rootDirectory / project.assetsDirectoryName;

	if (Filesystem::Exists(assetsDirectoryPath))
	{
		for (auto& pathEntry : Filesystem::RecursiveDirectoryIterator(assetsDirectoryPath))
		{
			if (pathEntry.path.Extension() == MetafileFileExtension)
			{
				AssetMetadata& newMetadata = outMetadata.emplace_back();
				
				YAMLFileStreamReader metadataReader;
				if (!metadataReader.OpenFile(pathEntry.path))
				{
					continue;
				}

				newMetadata.handle = metadataReader.ReadAtKey("Handle", AssetHandle(0));
				newMetadata.filepath = metadataReader.ReadAtKey("Path", Filesystem::Path(""));
				newMetadata.type = AssetTypes::None;

				const Filesystem::Path absoluteAssetPath = project.rootDirectory / newMetadata.filepath;
				if (Filesystem::Exists(absoluteAssetPath))
				{
					const String fileExtension = absoluteAssetPath.Extension().ToString();
					if (g_fileExtensionToAssetType.contains(fileExtension))
					{
						newMetadata.type = g_fileExtensionToAssetType.at(fileExtension);
					}
				}
			}
		}
	}
}

Vector<AssetReference<Volt::Asset>> LegacyProjectUpgrade::CreateMaterials(const Volt::Project& project, const Volt::AssetMetadata& metadata, MaterialsMap& materialsMap)
{
	// Since materials at this time had sub materials, 
	// we will create a material per sub material.
	
	const Filesystem::Path absoluteMaterialPath = project.rootDirectory / metadata.filepath;

	if (!Filesystem::Exists(absoluteMaterialPath))
	{
		return {};
	}

	YAMLFileStreamReader streamReader;
	if (!streamReader.OpenFile(absoluteMaterialPath))
	{
		return {};
	}

	if (!streamReader.HasKey("Material"))
	{
		return {};
	}

	streamReader.EnterScope("Material");

	const String materialName = streamReader.ReadAtKey("name", String("Null"));
	VT_LOG(Trace, "Material {}", materialName);

	MaterialDeclaration& materialDeclaration = materialsMap[metadata.handle];
	Vector<AssetReference<Volt::Asset>> newMaterials;

	streamReader.ForEach("materials", [&]() 
	{
		const String subMaterialName = streamReader.ReadAtKey("material", String("Null"));
		const uint32_t materialIndex = streamReader.ReadAtKey("index", 0u);
		const String shaderName = streamReader.ReadAtKey("shader", String("None"));

		//const uint32_t materialFlags = streamReader.ReadAtKey("flags", 0);
		//const bool isPermutation = streamReader.ReadAtKey("isPermutation", false);

		const String assetName = materialName + "_" + subMaterialName;
		AssetReference<Volt::MaterialAsset> material = g_assetManager->CreateAssetAndFile<Volt::MaterialAsset>(metadata.filepath.ParentPath(), assetName);
		// Since all materials were assumued to be alpha masked, we will set all materials to be
		// alpha masked here as well.

		if (shaderName == "IllumTransparent")
		{
			material->SetMaterialBlendMode(MaterialBlendMode::Translucent);
		}
		else if (shaderName == "Foliage")
		{
			material->SetMaterialBlendMode(MaterialBlendMode::AlphaMasked);
			material->SetIsDoubleSided(true);
		}
		else
		{
			material->SetMaterialBlendMode(MaterialBlendMode::AlphaMasked);
		}

		Ref<MaterialGraph> materialGraph = material->GetMaterialGraph();
		Mosaic::MosaicGraphBuilder mosaicBuilder(materialGraph->GetMosaicGraphMutable());

		UUID64 albedoTextureNode = 0;
		UUID64 materialTextureNode = 0;
		UUID64 normalTextureNode = 0;

		if (streamReader.HasKey("textures"))
		{
			streamReader.ForEach("textures", [&]()
			{
				const String binding = streamReader.ReadAtKey("binding", String());
				const AssetHandle handle = streamReader.ReadAtKey("handle", Asset::Null());

				// Skip null textures.
				if (handle == 0)
				{
					return;
				}

				if (binding == "albedo")
				{
					albedoTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
					MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(albedoTextureNode);
					textureNode.SetTextureHandle(handle);
				}
				else if (binding == "material")
				{
					materialTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
					MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(materialTextureNode);
					textureNode.SetTextureHandle(handle);
				}
				else if (binding == "normal")
				{
					normalTextureNode = mosaicBuilder.AddNode<MosaicNodes::SampleTextureNode>();
					MosaicNodes::SampleTextureNode& textureNode = mosaicBuilder.GetNodeAsType<MosaicNodes::SampleTextureNode>(normalTextureNode);
					textureNode.SetTextureType(MosaicNodes::TextureType::Normal);
					textureNode.SetTextureHandle(handle);
				}
			});
		}

		// Optional
		UUID64 baseColorFactorNode = 0;
		UUID64 roughnessFactorNode = 0;
		UUID64 metalnessNode = 0;

		UUID64 emissiveColorNode = mosaicBuilder.AddNode<MosaicNodes::Color3>();
		UUID64 emissiveStrengthColorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
		UUID64 normalStrengthNode = mosaicBuilder.AddNode<MosaicNodes::NormalStrengthNode>();

		if (streamReader.HasKey("data"))
		{
			streamReader.EnterScope("data");

			// Color
			{
				baseColorFactorNode = mosaicBuilder.AddNode<MosaicNodes::Color4>();
				mosaicBuilder.SetNodeParameterData(baseColorFactorNode, "RGBA", streamReader.ReadAtKey("color", glm::vec4(1.f)));
			}

			// No material texture
			if (materialTextureNode == 0)
			{
				roughnessFactorNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
				metalnessNode = mosaicBuilder.AddNode<MosaicNodes::ConstantFloat>();
				mosaicBuilder.SetNodeParameterData(roughnessFactorNode, "Value", streamReader.ReadAtKey("roughness", 0.5f));
				mosaicBuilder.SetNodeParameterData(metalnessNode, "Value", streamReader.ReadAtKey("metalness", 0.f));
			}

			mosaicBuilder.SetNodeParameterData(emissiveColorNode, "RGB", streamReader.ReadAtKey("emissiveColor", glm::vec3(1.f)));
			mosaicBuilder.SetNodeParameterData(emissiveStrengthColorNode, "Value", streamReader.ReadAtKey("emissiveStrength", 1.f));
			mosaicBuilder.SetNodeParameterData(normalStrengthNode, "Strength", streamReader.ReadAtKey("normalStrength", 0.f));
			streamReader.ExitScope();
		}

		UUID64 pbrOutputNode = mosaicBuilder.AddNode<MosaicNodes::PBROutputNode>();

		// Base color
		{

			if (albedoTextureNode != 0)
			{
				UUID64 multiplyNode = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(albedoTextureNode, multiplyNode, "RGBA", "A");
				mosaicBuilder.LinkNodeParameters(baseColorFactorNode, multiplyNode, "RGBA", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode, pbrOutputNode, "", "Base Color");
			}
			else
			{
				mosaicBuilder.LinkNodeParameters(baseColorFactorNode, pbrOutputNode, "RGBA", "Base Color");
			}
		}

		// Material
		{
			if (materialTextureNode != 0)
			{
				mosaicBuilder.LinkNodeParameters(materialTextureNode, pbrOutputNode, "R", "Metallic");
				mosaicBuilder.LinkNodeParameters(materialTextureNode, pbrOutputNode, "G", "Roughness");
			}
			else
			{
				mosaicBuilder.LinkNodeParameters(metalnessNode, pbrOutputNode, "Value", "Metallic");
				mosaicBuilder.LinkNodeParameters(roughnessFactorNode, pbrOutputNode, "Value", "Roughness");
			}

			UUID64 multiplyNode0 = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
			mosaicBuilder.LinkNodeParameters(emissiveColorNode, multiplyNode0, "RGB", "A");
			mosaicBuilder.LinkNodeParameters(emissiveStrengthColorNode, multiplyNode0, "Value", "B");

			if (materialTextureNode != 0)
			{
				UUID64 multiplyNode1 = mosaicBuilder.AddNode<MosaicNodes::MultiplyNode>();
				mosaicBuilder.LinkNodeParameters(multiplyNode0, multiplyNode1, "", "A");
				mosaicBuilder.LinkNodeParameters(materialTextureNode, multiplyNode1, "B", "B");
				mosaicBuilder.LinkNodeParameters(multiplyNode1, pbrOutputNode, "", "Emissive");
			}
			else
			{
				mosaicBuilder.LinkNodeParameters(multiplyNode0, pbrOutputNode, "", "Emissive");
			}
		}

		// Normal
		{
			UUID64 makeFloat2Node = mosaicBuilder.AddNode<MosaicNodes::ConversionMakeFloat2>();

			if (normalTextureNode != 0)
			{
				mosaicBuilder.LinkNodeParameters(normalTextureNode, makeFloat2Node, "A", "R");
				mosaicBuilder.LinkNodeParameters(normalTextureNode, makeFloat2Node, "G", "G");
			}
			else
			{
				mosaicBuilder.SetNodeInputParameterData(makeFloat2Node, "R", 0.5f);
				mosaicBuilder.SetNodeInputParameterData(makeFloat2Node, "G", 0.5f);
			}

			UUID64 deriveNormalZNode = mosaicBuilder.AddNode<MosaicNodes::DeriveNormalZNode>();
			mosaicBuilder.LinkNodeParameters(makeFloat2Node, deriveNormalZNode, "Result", "XY");
			mosaicBuilder.LinkNodeParameters(deriveNormalZNode, normalStrengthNode, "Result", "Normal");
			mosaicBuilder.LinkNodeParameters(normalStrengthNode, pbrOutputNode, "Result", "Normal");
		}

		if (materialDeclaration.subMaterials.size() <= materialIndex)
		{
			materialDeclaration.subMaterials.resize(materialIndex + 1);
		}

		materialDeclaration.subMaterials[materialIndex] = material->GetAssetHandle();
		newMaterials.emplace_back(material);

		VT_LOG(Trace, "Sub Material {}", subMaterialName);
	});


	streamReader.ExitScope();

	return newMaterials;
 }

 AssetReference<Volt::Asset> LegacyProjectUpgrade::TryConvertTexture(const Volt::Project& project, const Volt::AssetMetadata& metadata)
 {
	 const Filesystem::Path absoluteTexturePath = project.rootDirectory / metadata.filepath;

	 if (!Filesystem::Exists(absoluteTexturePath))
	 {
		 return {};
	 }

	 Volt::TextureSourceImportConfig importConfig;
	 importConfig.destinationDirectory = m_targetDirectory / metadata.filepath.ParentPath();
	 importConfig.destinationFilename = metadata.filepath.Stem().ToString();

	 // If it's a HDR file, it should be imported as an environment texture.
	 if (metadata.filepath.Extension() == ".hdr")
	 {
		 importConfig.createAsMemoryAsset = true;
		 JobFuture<Vector<AssetReference<Asset>>> future = SourceAssetManager::ImportSourceAsset(absoluteTexturePath, importConfig);

		 Volt::Renderer::EnvironmentTextures envTextures = Volt::Renderer::GenerateEnvironmentTextures(future.Get().front()->GetAssetHandle());
		 return g_assetManager->CreateAssetAndFileWithAssetHandle<Volt::EnvironmentTexture>(importConfig.destinationDirectory, importConfig.destinationFilename, metadata.handle, envTextures.diffuse, envTextures.specular);
	 }
	 else
	 {
		 importConfig.generateMipMaps = true;
		 importConfig.importMipMaps = true;
		 importConfig.compressionType = TextureCompressionType::BC5;
		 importConfig.targetAssetHandle = metadata.handle;

		 JobFuture<Vector<AssetReference<Asset>>> future = SourceAssetManager::ImportSourceAsset(absoluteTexturePath, importConfig);
		 return future.Get().front();
	 }
 }

 LegacyProjectUpgrade::~LegacyProjectUpgrade()
 {
	 if (m_assetManager)
	 {
		 g_assetManager = std::move(m_assetManager);
	 }
 }
