#include "Upgrades/0_1_6.h"

#include "UpgradesRegistry.h"

#include "Volt-Platforms/Windows/WindowsPlatformThread.h"

#include <CoreUtilities/FileSystem.h>

#include <CoreUtilities/FileIO/YAMLMemoryStreamReader.h>
#include <CoreUtilities/FileIO/YAMLMemoryStreamWriter.h>

#include <Volt-Core/Project/Project.h>

namespace Volt
{
	REGISTER_UPGRADE(Volt::Version::Create(0, 1, 6), Upgrade_0_1_6);

	Upgrade_0_1_6::Upgrade_0_1_6(const Project& inProject)
		: Upgrade(inProject)
	{
		m_currentStage = UpgradeStage::Collecting;
		m_numActionsCompleted = 0;
		m_numTotalActions = 0;
	}

	bool Upgrade_0_1_6::ProcessUpgrade()
	{
		switch (m_currentStage)
		{
			case UpgradeStage::Collecting:
			{
				const auto assetsDir = GetTargetProject().rootDirectory / GetTargetProject().assetsDirectory;
				if (FileSystem::Exists(assetsDir))
				{
					for (auto& p : std::filesystem::recursive_directory_iterator(assetsDir))
					{
						if (p.path().extension() == ".vtent" ||
							p.path().extension() == ".vtasset")
						{
							m_filesToProcess.emplace_back(p.path());
						}

						if (p.path().extension() == ".vtasset")
						{
							BinaryStreamReader streamReader{ p.path() };
							if (!streamReader.IsStreamValid())
							{
								continue;
							}

							uint32_t magicVal = 0;
							streamReader.Read(magicVal);
							if (magicVal != OldSerializedAssetMetadata::AssetMagic)
							{
								continue;
							}
							OldSerializedAssetMetadata OldMetadata;
							streamReader.Read(OldMetadata);

							//if asset is a scene
							if (OldMetadata.type == "{EF155FF1-61DC-4200-84DE-4A0C8A01D049}"_guid)
							{
								m_sceneFilesToProcess.push_back(p.path());
							}
						}
					}
				}
				m_numTotalActions = m_filesToProcess.size() + m_sceneFilesToProcess.size();
				m_currentStage = UpgradeStage::Converting;
				break;
			}
			case UpgradeStage::Converting:
			{
				//25 is probably good enough
				for (size_t i = 0; i < 25; i++)
				{
					if (m_filesToProcess.empty())
					{
						break;
					}
					std::filesystem::path path = m_filesToProcess.back();
					m_filesToProcess.pop_back();

					ProcessFile(path);
				}

				if (m_filesToProcess.empty())
				{
					m_currentStage = UpgradeStage::MovingSceneFiles;
				}
				break;
			}

			case UpgradeStage::MovingSceneFiles:
			{
				//5 is probably good enough
				for (size_t i = 0; i < 5; i++)
				{
					if (m_sceneFilesToProcess.empty())
					{
						break;
					}
					std::filesystem::path path = m_sceneFilesToProcess.back();
					m_sceneFilesToProcess.pop_back();

					MoveSceneFileAndEntities(path);
					m_numActionsCompleted++;
				}

				if (m_sceneFilesToProcess.empty())
				{
					m_currentStage = UpgradeStage::Done;
				}
				break;
			}
		}

		return m_currentStage == UpgradeStage::Done;
	}

	size_t Upgrade_0_1_6::GetNumTotalActions()
	{
		return m_numTotalActions;
	}

	size_t Upgrade_0_1_6::GetNumActionsCompleted()
	{
		return m_numActionsCompleted;
	}

	std::string Upgrade_0_1_6::GetCurrentActionText()
	{
		switch (m_currentStage)
		{
			case UpgradeStage::Collecting:
				return "Collecting files to convert...";
			case UpgradeStage::Converting:
				return "Converting files...";
			case UpgradeStage::MovingSceneFiles:
				return "Moving Scene And Entity Files...";
		}
		return "Error";
	}

	void Upgrade_0_1_6::ProcessFile(std::filesystem::path inPath)
	{
		if (!FileSystem::IsWriteable(inPath))
		{
			FileSystem::MakeWriteable(inPath);
		}

		if (inPath.extension() == ".vtent")
		{
			ProcessEntityFile(inPath);
		}
		else if (inPath.extension() == ".vtasset")
		{
			ProcessAssetFile(inPath);
		}

		m_numActionsCompleted++;
	}

	void Upgrade_0_1_6::ProcessEntityFile(std::filesystem::path inPath)
	{
		//find the owning scene, it should be one folder up and the only vtasset in that folder
		const std::filesystem::path sceneDir = inPath.parent_path().parent_path();
		std::filesystem::path sceneAssetPath;
		if (FileSystem::Exists(sceneDir))
		{
			for (auto& p : std::filesystem::directory_iterator(sceneDir))
			{
				if (p.path().extension() == ".vtasset")
				{
					sceneAssetPath = p;
					break;
				}
			}
		}

		//get the asset handle of the owning scene
		BinaryStreamReader sceneStreamReader{ sceneAssetPath };
		if (!sceneStreamReader.IsStreamValid())
		{
			return;
		}

		uint32_t magicSceneVal = 0;
		sceneStreamReader.Read(magicSceneVal);
		if (magicSceneVal != OldSerializedAssetMetadata::AssetMagic)
		{
			return;
		}
		OldSerializedAssetMetadata OldMetadata;
		sceneStreamReader.Read(OldMetadata);

		UUID64 OwningSceneAssetHandle = OldMetadata.handle;
		//done finding owning scene

		BinaryStreamReader streamReader{ inPath };
		if (!streamReader.IsStreamValid())
		{
			return;
		}

		uint32_t magicVal = 0;
		streamReader.Read(magicVal);

		constexpr uint32_t ENTITY_MAGIC_VAL = 1515;
		if (magicVal != ENTITY_MAGIC_VAL)
		{
			return;
		}

		//this buffer is the entity data
		Buffer buffer{};
		streamReader.Read(buffer);


		//write to file
		BinaryStreamWriter entityDescFileWriter{};

		entityDescFileWriter.Write(NewSerializedAssetMetadata::AssetMagic);

		NewSerializedAssetMetadata newMetadata;
		newMetadata.handle = UUID64();
		newMetadata.type = "{50C26090-1874-4609-8386-67AEB44CE208}"_guid;
		newMetadata.version = 1;


		newMetadata.customData.resize(sizeof(EntityDescCustomMetadata));
		memset(newMetadata.customData.data(), 0, ASSET_CUSTOM_METADATA_SIZE);

		//custom metadata for entities contain the sceneHandle of the entity
		{
			UUID64& MetadataSceneHandleRef = *reinterpret_cast<UUID64*>(newMetadata.customData.data());
			//assign scene handle here
			MetadataSceneHandleRef = OwningSceneAssetHandle;
		}

		//they also contain the ID of the entity
		{
			uint32_t& entityIDRef = *reinterpret_cast<uint32_t*>(newMetadata.customData.data() + sizeof(UUID64));

			YAMLMemoryStreamReader reader;
			reader.ReadBuffer(buffer);

			reader.EnterScope("Entity");
			entityIDRef = reader.ReadAtKey("id", static_cast<uint32_t>(0));
			VT_ASSERT(entityIDRef != 0);
		}

		size_t compressedDataOffset = entityDescFileWriter.Write(newMetadata);

		entityDescFileWriter.Write(buffer);
		buffer.Release();

		entityDescFileWriter.WriteToDisk(inPath, true, compressedDataOffset);

		//change extension of entity file
		std::filesystem::path newPath = inPath;
		newPath.replace_filename(inPath.stem().string() + ".vtasset");
		std::filesystem::rename(inPath, newPath);
	}

	void Upgrade_0_1_6::ProcessAssetFile(std::filesystem::path inPath)
	{
		BinaryStreamReader streamReader{ inPath };
		if (!streamReader.IsStreamValid())
		{
			return;
		}

		uint32_t magicVal = 0;
		streamReader.Read(magicVal);
		if (magicVal != OldSerializedAssetMetadata::AssetMagic)
		{
			return;
		}
		OldSerializedAssetMetadata OldMetadata;
		streamReader.Read(OldMetadata);

		//this is the asset data
		Vector<uint8_t> bytes;
		streamReader.ReadBytesRaw(bytes, streamReader.GetRemainingDataSize());


		//write to file
		BinaryStreamWriter streamWriter{};

		NewSerializedAssetMetadata newMetadata{};
		newMetadata.handle = OldMetadata.handle;
		newMetadata.version = OldMetadata.version;
		newMetadata.type = OldMetadata.type;

		//call reserve here to set the begin ptr
		newMetadata.customData.reserve(ASSET_CUSTOM_METADATA_SIZE);
		memset(newMetadata.customData.data(), 0, ASSET_CUSTOM_METADATA_SIZE);

		streamWriter.Write(NewSerializedAssetMetadata::AssetMagic);
		size_t compressedDataOffset = streamWriter.Write(newMetadata);

		streamWriter.WriteWithoutHeader(bytes.data(), bytes.size());

		streamWriter.WriteToDisk(inPath, true, compressedDataOffset);
	}

	void Upgrade_0_1_6::MoveSceneFileAndEntities(std::filesystem::path inPath)
	{
		std::filesystem::path newSceneDirectory = inPath.parent_path().parent_path();
		FileSystem::Move(inPath, newSceneDirectory);


		std::filesystem::path oldEntitiesDir = inPath.parent_path() / "Entities";
		Vector<std::filesystem::path> entitiesPaths;

		for (auto& p : std::filesystem::recursive_directory_iterator(oldEntitiesDir))
		{
			entitiesPaths.push_back(p.path());
		}

		const std::filesystem::path& newEntityDirectory = newSceneDirectory / (inPath.stem().string() + "_Entities");
		FileSystem::CreateDirectories(newEntityDirectory);
		for (const std::filesystem::path& entityPath : entitiesPaths)
		{
			FileSystem::Move(entityPath, newEntityDirectory);
		}

		//remove the folder that the old scene lived in
		FileSystem::Remove(inPath.parent_path());
	}

	void Upgrade_0_1_6::NewSerializedAssetMetadata::Serialize(BinaryStreamWriter& streamWriter, const NewSerializedAssetMetadata& data)
	{
		streamWriter.Write(data.handle);
		streamWriter.Write(data.type);
		streamWriter.Write(data.version);
		streamWriter.Write(data.customData);
	}

	void Upgrade_0_1_6::OldSerializedAssetMetadata::Deserialize(BinaryStreamReader& streamReader, OldSerializedAssetMetadata& outData)
	{
		streamReader.Read(outData.handle);
		streamReader.Read(outData.type);
		streamReader.Read(outData.version);
	}

}
