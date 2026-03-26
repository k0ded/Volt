#pragma once
#include "UpgradeInterface.h"

#include "ProjectUpgradeClient/Common/BinaryStreamReader.h"
#include "ProjectUpgradeClient/Common/BinaryStreamWriter.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/UUID.h>

#include <filesystem>

namespace Volt
{
	struct OldSerializedAssetMetadata
	{
		inline static constexpr uint32_t AssetMagic = 9999;
		inline static constexpr size_t HeaderSize = sizeof(VoltGUID) + sizeof(uint32_t) + sizeof(UUID64) + sizeof(TypeHeader) * 9;

		VoltGUID type;
		uint32_t version;
		UUID64 handle;
	};
	static void Deserialize(BinaryStreamReader& streamReader, OldSerializedAssetMetadata& outData);

	static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
	struct NewSerializedAssetMetadata
	{
		inline static constexpr uint32_t AssetMagic = 9999;
		inline static constexpr size_t HeaderSize = sizeof(VoltGUID) + sizeof(uint32_t) + sizeof(UUID64) + sizeof(TypeHeader) * 9 + ASSET_CUSTOM_METADATA_SIZE + sizeof(TypeHeader) * 2;

		VoltGUID type;
		uint32_t version;
		UUID64 handle;

		Vector<uint8_t, InlineAllocator<ASSET_CUSTOM_METADATA_SIZE>> customData; // asset specific Metadata
	};
	static void Serialize(BinaryStreamWriter& streamWriter, const NewSerializedAssetMetadata& data);

	class Upgrade_0_1_6 : public Upgrade
	{
	public:
		Upgrade_0_1_6(const Project& inProject);
		virtual ~Upgrade_0_1_6() = default;

		bool ProcessUpgrade() override;

		// Inherited via Upgrade
		size_t GetNumTotalActions() override;
		size_t GetNumActionsCompleted() override;

		String GetCurrentActionText() override;

	private:
		enum class UpgradeStage
		{
			Collecting,
			Converting,
			MovingSceneFiles,
			Done
		};

		struct EntityDescCustomMetadata
		{
			UUID64 sceneHandle;
			uint32_t entityID;
		};
	private:
		void ProcessFile(Filesystem::Path inPath);
		void ProcessEntityFile(Filesystem::Path inPath);
		void ProcessAssetFile(Filesystem::Path inPath);
		void MoveSceneFileAndEntities(Filesystem::Path inPath);

		UpgradeStage m_currentStage;
		//absolute paths
		Vector<Filesystem::Path> m_filesToProcess;
		Vector<Filesystem::Path> m_sceneFilesToProcess;

		size_t m_numTotalActions;
		size_t m_numActionsCompleted;
	};
}
