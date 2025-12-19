#include "EntityDescriptionSerializer.h"

#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializerRegistry.h"

#define private public
#include <Volt-Scene/EntityDescription.h>
#undef private
#include <Volt-Scene/EntityDescCustomMetadata.h>
#include <Volt-Scene/Scene.h>

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/AssetLocks.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <EntitySystem/Entity.h>
#include <EntitySystem/ComponentRegistry.h>
#include <EntitySystem/Scripting/CoreComponents.h>

#include <Volt-Platforms/Windows/WindowsPlatformThread.h>

#include <CoreUtilities/Containers/Map.h>

namespace Volt
{
	Map<VoltGUID, Map<std::string, uint32_t>> g_componentMemberRemap =
	{
		// TagComponent
		{
			"{282FA5FB-6A77-47DB-8340-3D34F1A1FBBD}"_guid,
			{
				{ "tag", 'tag' }
			}
		},
		// IDComponent
		{
			"{663E0E0B-43EC-4973-8A9B-FF8A0BA566AA}"_guid,
			{
				{ "id", 'id' }
			}
		},
		// TransformComponent
		{
			"{E1B8016B-1CAA-4782-927E-C17C29B25893}"_guid,
			{
				{ "position", 'pos' },
				{ "rotation", 'rot' },
				{ "scale", 'scal' },
				{ "visible", 'vis' },
				{ "locked", 'lock' },
				{ "movability", 'mvbl' },
			}
		},
		// RelationshipComponent
		{
			"{4A5FEDD2-4D0B-4696-A9E6-DCDFFB25B32C}"_guid,
			{
				{ "parent", 'par' },
				{ "children", 'chld' },
			}
		},
		// CommonComponent
		{
			"{A6789316-2D82-46FC-8138-B7BCBB9EA5B8}"_guid,
			{
				{ "timecreatedid", 'time' }
			}
		},
		// RigidbodyComponent
		{
			"{460B7722-00C0-48BE-8B3E-B549BCC9269B}"_guid,
			{
				{ "bodyType", 'bdtp' },
				{ "layerId", 'lyrd' },
				{ "mass", 'mass' },
				{ "linearDrag", 'lndr' },
				{ "lockFlags", 'lcfl' },
				{ "angularDrag", 'andr' },
				{ "collisionType", 'colt' },
				{ "disableGravity", 'dsgr' },
				{ "isKinematic", 'iskn' }
			}
		},
		// BoxColliderComponent
		{
			"{29707475-D536-4DA4-8D3A-A98948C89A5}"_guid,
			{
				{ "halfSize", 'hasi' },
				{ "offset", 'offs' },
				{ "isTrigger", 'istr' },
				{ "material", 'mat' }
			}
		},
		// SphereColliderComponent
		{
			"{90246BCE-FF83-41A2-A076-AB0A947C0D6A}"_guid,
			{
				{ "radius", 'radi' },
				{ "offset", 'offs' },
				{ "isTrigger", 'istr' },
				{ "material", 'mat' }
			}
		},
		// CapsuleColliderComponent
		{
			"{54A48952-7A77-492B-8A9C-2440D82EE5E2}"_guid,
			{
				{ "radius", 'radi' },
				{ "height", 'heig' },
				{ "offset", 'offs' },
				{ "isTrigger", 'istr' },
				{ "material", 'mat' }
			}
		},
		// MeshColliderComponent
		{
			"{E709C708-ED3C-4F68-BC1D-2FE32B897722}"_guid,
			{
				{ "colliderMesh", 'clme' },
				{ "material", 'mat' },
				{ "subMeshIndex", 'smi' },
				{ "isConvex", 'isco' },
				{ "isTrigger", 'istr' }
			}
		},
		// CharacterControllerComponent
		{
			"{DC5C002A-B72E-42A0-83FC-FFBE1FB2DEF2}"_guid,
			{
				{ "climbingMode", 'clim' },
				{ "slopeLimit", 'slli' },
				{ "invisibleWallHeight", 'iwh' },
				{ "maxJumpHeight", 'mjh' },
				{ "contactOffset", 'coff' },
				{ "stepOffset", 'soff' },
				{ "density", 'dens' },
				{ "layer", 'layr' },
				{ "hasGravity", 'hasg' }
			}
		},
		// PointLightComponent
		{
			"{A30A8848-A30B-41DD-80F9-4E163C01ABC2}"_guid,
			{
				{ "intensity", 'inte' },
				{ "radius", 'radi' },
				{ "falloff", 'fall' },
				{ "color", 'col' },
				{ "castShadows", 'shdw' }
			}
		},
		// SpotLightComponent
		{
			"{D35F915F-53E5-4E15-AE5B-769F4D79B6F8}"_guid,
			{
				{ "intensity", 'inte' },
				{ "innerAngle", 'angi' },
				{ "outerAngle", 'ango' },
				{ "range", 'rang' },
				{ "falloff", 'fall' },
				{ "color", 'col' },
				{ "castShadows", 'shdw' }
			}
		},
		// SphereLightComponent
		{
			"{0D0CEEE2-A331-442A-BB4B-FBDB8E06C692}"_guid,
			{
				{ "intensity", 'inte' },
				{ "radius", 'radi' },
				{ "color", 'col' }
			}
		},
		// RectangleLightComponent
		{
			"{5AEF9201-4A86-45F1-85F3-E95577E45BF2}"_guid,
			{
				{ "intensity", 'inte' },
				{ "color", 'col' },
				{ "width", 'wid' },
				{ "height", 'heig' }
			}
		},
		// DirectionalLightComponent
		{
			"{EC5514FF-9DE7-44CA-BCD9-8A9F08883F59}"_guid,
			{
				{ "intensity", 'inte' },
				{ "color", 'col' },
				{ "lightSize", 'lisz' },
				{ "sunRadius", 'snrd' },
				{ "softShadows", 'sfsh' },
				{ "castShadows", 'shdw' }
			}
		},
		// SkylightComponent
		{
			"{29F75381-2873-4734-A074-3F3640E54C84}"_guid,
			{
				{ "environmentHandle", 'env' },
				{ "intensity", 'inte' },
				{ "lod", 'lod' },
				{ "show", 'show' }
			}
		},
		// AnimationPlayerComponent
		{
			"{45673840-5218-417D-A1C7-800A46711F23}"_guid,
			{
				{ "skeleton", 'skel' },
				{ "animationHandle", 'anim' },
				{ "currentPlayTime", 'play' }
			}
		},
		// MeshComponent
		{
			"{45D008BE-65C9-4D6F-A0C6-377F7B384E47}"_guid,
			{
				{ "handle", 'hndl' },
				{ "materials", 'mats' }
			}
		},
		// CameraComponent
		{
			"{9258BEEC-3A31-4CAB-AB1E-654524E1C398}"_guid,
			{
				{ "fieldOfView", 'fov' },
				{ "nearPlane", 'nrpl' },
				{ "farPlane", 'frpl' },
				{ "priority", 'prio' }
			}
		},
		// TextRendererComponent
		{
			"{8AAA0646-40D2-47E6-B83F-72EA26BD8C01}"_guid,
			{
				{ "text", 'text' },
				{ "font", 'font' },
				{ "maxWidth", 'mxwd' },
				{ "color", 'col' }
			}
		},
		// SpriteComponent
		{
			"{FDB47734-1B69-4558-B460-0975365DB400}"_guid,
			{
				{ "materialHandle", 'hndl' }
			}
		},
		// DecalComponent
		{
			"{09FA1C73-D508-4ADA-A101-A63703E91345}"_guid,
			{
				{ "decalMaterial", 'dcl' }
			}
		},
		// PrefabComponent
		{
			"{B8A83ACF-F1CA-4C9F-8D1E-408B5BB388D2}"_guid,
			{
				{ "prefabAsset", 'prea' },
				{ "prefabEntity", 'pree' },
				{ "sceneRootEntity", 'sre' },
				{ "version", 'ver' },
				{ "componentLocalChanges", 'clc' }
			}
		},
	};

	VT_REGISTER_ASSET_SERIALIZER(AssetTypes::EntityDesc, EntityDescSerializer);

	template<typename T>
	void RegisterSerializationFunction(std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamWriter&, const uint8_t*, const size_t)>>& outTypes)
	{
		VT_PROFILE_FUNCTION();

		outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLMemoryStreamWriter& streamWriter, const uint8_t* data, const size_t offset)
		{
			const T& var = *reinterpret_cast<const T*>(&data[offset]);
			streamWriter.SetKey("data", var);
		};
	}

	template<typename T>
	void RegisterDeserializationFunction(std::unordered_map<TypeTraits::TypeIndex, std::function<void(YAMLMemoryStreamReader&, uint8_t*, const size_t)>>& outTypes)
	{
		VT_PROFILE_FUNCTION();

		outTypes[TypeTraits::TypeIndex::FromType<T>()] = [](YAMLMemoryStreamReader& streamReader, uint8_t* data, const size_t offset)
		{
			*reinterpret_cast<T*>(&data[offset]) = streamReader.ReadAtKey("data", T());
		};
	}

	EntityDescSerializer::EntityDescSerializer()
	{
		RegisterDeserializationFunction<int8_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint8_t>(m_typeDeserializers);
		RegisterDeserializationFunction<int16_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint16_t>(m_typeDeserializers);
		RegisterDeserializationFunction<int32_t>(m_typeDeserializers);
		RegisterDeserializationFunction<uint32_t>(m_typeDeserializers);

		RegisterDeserializationFunction<float>(m_typeDeserializers);
		RegisterDeserializationFunction<double>(m_typeDeserializers);
		RegisterDeserializationFunction<bool>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::vec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::vec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::vec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::uvec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::uvec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::ivec2>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec3>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::ivec4>(m_typeDeserializers);

		RegisterDeserializationFunction<glm::quat>(m_typeDeserializers);
		RegisterDeserializationFunction<glm::mat4>(m_typeDeserializers);
		RegisterDeserializationFunction<VoltGUID>(m_typeDeserializers);

		RegisterDeserializationFunction<std::string>(m_typeDeserializers);
		RegisterDeserializationFunction<std::filesystem::path>(m_typeDeserializers);

		RegisterDeserializationFunction<Volt::EntityID>(m_typeDeserializers);
		RegisterDeserializationFunction<AssetHandle>(m_typeDeserializers);

		s_instance = this;
	}

	EntityDescSerializer::~EntityDescSerializer()
	{}

	void EntityDescSerializer::Serialize(const AssetMetadata_0_1_7* metadata, CustomAssetMetadataVector& customData, const AssetReference<Asset>& asset) const
	{
	}

	bool EntityDescSerializer::Deserialize(const AssetMetadata_0_1_7* metadata, AssetReference<Asset> destinationAsset) const
	{
		AssetReference<EntityDesc> entityDesc = destinationAsset.ConvertTo<EntityDesc>();
		ScopedAssetReferenceLock entityDescLock{ entityDesc };

		const auto filePath = g_assetManager->GetAssetFilesystemPath(metadata->filepath);

		if (!std::filesystem::exists(filePath))
		{
			VT_LOG(Error, "File {0} not found!", metadata->filepath);
			entityDesc->SetFlag(AssetFlag::Missing, true);
			return false;
		}

		BinaryStreamReader streamReader{ filePath };
		if (!streamReader.IsStreamValid())
		{
			VT_LOG(Error, "Failed to open file {0}!", metadata->filepath);
			entityDesc->SetFlag(AssetFlag::Invalid, true);
			return false;
		}

		const EntityDescCustomMetadata& customMeta = metadata->GetCustomData<EntityDescCustomMetadata>();
		
		entityDesc->m_sceneHandle = customMeta.sceneHandle;
		entityDesc->m_entityID = customMeta.entityID;

		AssetSerializer::ReadMetadata(streamReader);

		entityDesc->m_entitySpawnData.Clear();
		streamReader.Read(entityDesc->m_entitySpawnData);		
		return true;
	}

	Entity EntityDescSerializer::DeserializeEntity(AssetReference<Scene> scene, YAMLMemoryStreamReader& streamReader) const
	{
		streamReader.EnterScope("Entity");

		EntityID entityId = streamReader.ReadAtKey("id", Entity::NullID());

		if (entityId == Entity::NullID())
		{
			return Entity::Null();
		}

		ScopedAssetReferenceLock sceneLock{ scene };

		Entity entity = scene->CreateEntityWithID(entityId);

		streamReader.ForEach("components", [&]()
		{
			VT_PROFILE_SCOPE("Component");

			VoltGUID compGuid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (compGuid == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(compGuid);
			if (!typeDesc)
			{
				return;
			}

			switch (typeDesc->GetValueType())
			{
				case ValueType::Component:
				{
					auto& registry = scene->GetEntityScene().GetRegistry();

					if (!ComponentRegistry::Helpers::HasComponentWithGUID(compGuid, registry, entity.GetHandle()))
					{
						ComponentRegistry::Helpers::AddComponentWithGUID(compGuid, registry, entity.GetHandle());
					}

					void* voidCompPtr = ComponentRegistry::Helpers::GetComponentWithGUID(compGuid, registry, entity.GetHandle());
					uint8_t* componentData = reinterpret_cast<uint8_t*>(voidCompPtr);

					const IComponentTypeDesc* componentDesc = reinterpret_cast<const IComponentTypeDesc*>(typeDesc);
					DeserializeClass(componentData, 0, componentDesc, entity, streamReader);
					break;
				}
			}

		});

		streamReader.ExitScope();

		return entity;
	}

	Vector<VoltGUID> EntityDescSerializer::FindComponentTypes(YAMLMemoryStreamReader& streamReader)
	{
		Vector<VoltGUID> result;

		streamReader.EnterScope("Entity");

		streamReader.ForEach("components", [&]()
		{
			VT_PROFILE_SCOPE("Component");

			VoltGUID compGuid = streamReader.ReadAtKey("guid", VoltGUID::Null());
			if (compGuid == VoltGUID::Null())
			{
				return;
			}

			const ICommonTypeDesc* typeDesc = GetComponentRegistry().GetTypeDescFromGUID(compGuid);
			if (!typeDesc)
			{
				return;
			}

			result.push_back(compGuid);
		});

		streamReader.ExitScope();

		return result;
	}

	std::filesystem::path EntityDescSerializer::GetSavePathForEntity_ThreadSafe(const Volt::AssetHandle& handle)
	{
		ReadOnlyAssetMetadata assetMetadata = g_assetManager->GetReadOnlyAssetMetadata(handle);
		VT_ENSURE(assetMetadata->type == AssetTypes::EntityDesc);

		const EntityDescCustomMetadata& entityMetadata = assetMetadata->GetCustomData<EntityDescCustomMetadata>();

		ReadOnlyAssetMetadata sceneAssetMetadata = g_assetManager->GetReadOnlyAssetMetadata(entityMetadata.sceneHandle);

		VT_ENSURE(sceneAssetMetadata->HasFilepath());

		const std::filesystem::path& owningScenePath = sceneAssetMetadata->filepath;
		const std::string owningSceneName = owningScenePath.stem().string();

		const std::filesystem::path relativePath = owningScenePath.parent_path() / (owningSceneName + "_Entities") / (std::to_string(entityMetadata.entityID) + ".vtasset");
		return g_assetManager->GetAssetFilesystemPath(relativePath);
	}

	Entity EntityDescSerializer::CreateEntityFromUUIDThreadSafe(EntityID entityId, AssetReference<Scene> scene) const
	{
		static std::mutex createEntityMutex;
		std::scoped_lock lock{ createEntityMutex };

		return scene->CreateEntityWithID(entityId);
	}

	void EntityDescSerializer::DeserializeClass(uint8_t* data, const size_t offset, const IComponentTypeDesc* compDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const
	{
		streamReader.ForEach("members", [&]()
		{
			const std::string memberName = streamReader.ReadAtKey("name", std::string(""));
			if (memberName.empty())
			{
				return;
			}

			if (!g_componentMemberRemap.contains(compDesc->GetGUID()))
			{
				return;
			}

			if (!g_componentMemberRemap.at(compDesc->GetGUID()).contains(memberName))
			{
				return;
			}

			const ComponentMember* componentMember = const_cast<IComponentTypeDesc*>(compDesc)->FindMemberByIdentifier(g_componentMemberRemap.at(compDesc->GetGUID()).at(memberName));
			if (!componentMember)
			{
				return;
			}

			if (componentMember->typeDesc != nullptr)
			{
				switch (componentMember->typeDesc->GetValueType())
				{
					case ValueType::Component:
					{
						const IComponentTypeDesc* memberCompType = reinterpret_cast<const IComponentTypeDesc*>(componentMember->typeDesc);
						DeserializeClass(data, offset + componentMember->offset, memberCompType, dstEntity, streamReader);
						break;
					}

					case ValueType::Enum:
					{
						*reinterpret_cast<int32_t*>(&data[offset + componentMember->offset]) = streamReader.ReadAtKey("enumValue", int32_t(0));
						break;
					}

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(componentMember->typeDesc);
						DeserializeArray(data, offset + componentMember->offset, arrayTypeDesc, dstEntity, streamReader);
						break;
					}
				}
			}
			else
			{
				if (m_typeDeserializers.contains(componentMember->typeIndex))
				{
					m_typeDeserializers.at(componentMember->typeIndex)(streamReader, data, offset + componentMember->offset);
				}
			}
		});
	}

	void EntityDescSerializer::DeserializeArray(uint8_t* data, const size_t offset, const IArrayTypeDesc* arrayDesc, Entity dstEntity, YAMLMemoryStreamReader& streamReader) const
	{
		void* arrayPtr = &data[offset];

		const bool isNonDefaultType = arrayDesc->GetElementTypeDesc() != nullptr;
		const auto& typeIndex = arrayDesc->GetElementTypeIndex();

		if (!isNonDefaultType && !m_typeDeserializers.contains(typeIndex))
		{
			return;
		}

		streamReader.ForEach("values", [&]()
		{
			void* tempDataStorage = nullptr;
			arrayDesc->DefaultConstructElement(tempDataStorage);

			uint8_t* tempBytePtr = reinterpret_cast<uint8_t*>(tempDataStorage);

			if (isNonDefaultType)
			{
				switch (arrayDesc->GetElementTypeDesc()->GetValueType())
				{
					case ValueType::Component:
					{
						const IComponentTypeDesc* compType = reinterpret_cast<const IComponentTypeDesc*>(arrayDesc->GetElementTypeDesc());
						DeserializeClass(tempBytePtr, 0, compType, dstEntity, streamReader);
						break;
					}

					case ValueType::Enum:
						*reinterpret_cast<int32_t*>(&tempDataStorage) = streamReader.ReadAtKey("enumValue", int32_t(0));
						break;

					case ValueType::Array:
					{
						const IArrayTypeDesc* arrayTypeDesc = reinterpret_cast<const IArrayTypeDesc*>(arrayDesc->GetElementTypeDesc());
						DeserializeArray(tempBytePtr, 0, arrayTypeDesc, dstEntity, streamReader);
						break;
					}
				}
			}
			else
			{
				if (m_typeDeserializers.contains(typeIndex))
				{
					m_typeDeserializers.at(typeIndex)(streamReader, tempBytePtr, 0);
				}
			}

			arrayDesc->PushBack(arrayPtr, tempDataStorage);
			arrayDesc->DestroyElement(tempDataStorage);
		});
	}
}
