#pragma once
#include "UpgradeInterface.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/StackVector.h>
#include <CoreUtilities/VoltGUID.h>
#include <CoreUtilities/UUID.h>

#include <filesystem>

namespace Volt
{
	class Upgrade_0_1_6 : public Upgrade
	{
	public:
		Upgrade_0_1_6(const Project& inProject);
		virtual ~Upgrade_0_1_6() = default;

		bool ProcessUpgrade() override;

		// Inherited via Upgrade
		size_t GetNumTotalActions() override;
		size_t GetNumActionsCompleted() override;

		std::string GetCurrentActionText() override;

	private:
		enum class UpgradeStage
		{
			Collecting,
			Converting
		};

		struct OldSerializedAssetMetadata
		{
			inline static constexpr uint32_t AssetMagic = 9999;
			inline static constexpr size_t HeaderSize = sizeof(VoltGUID) + sizeof(uint32_t) + sizeof(UUID64) + sizeof(TypeHeader) * 9 ;

			VoltGUID type;
			uint32_t version;
			UUID64 handle;

			static void Deserialize(BinaryStreamReader& streamReader, OldSerializedAssetMetadata& outData);
		};

		static constexpr size_t ASSET_CUSTOM_METADATA_SIZE = 256;
		struct NewSerializedAssetMetadata
		{
			inline static constexpr uint32_t AssetMagic = 9999;
			inline static constexpr size_t HeaderSize = sizeof(VoltGUID) + sizeof(uint32_t) + sizeof(UUID64) + sizeof(TypeHeader) * 9 + sizeof(StackVector<uint8_t, ASSET_CUSTOM_METADATA_SIZE>) + sizeof(TypeHeader) * 2;

			VoltGUID type;
			uint32_t version;
			UUID64 handle;

			StackVector<uint8_t, ASSET_CUSTOM_METADATA_SIZE> customData; // asset specific Metadata

			static void Serialize(BinaryStreamWriter& streamWriter, const NewSerializedAssetMetadata& data);
		};
	private:
		void ProcessFile(std::filesystem::path inPath);
		void ProcessEntityFile(std::filesystem::path inPath);
		void ProcessAssetFile(std::filesystem::path inPath);


		UpgradeStage m_currentStage;
		//absolute paths
		Vector<std::filesystem::path> m_filesToProcess;

		size_t m_numTotalActions;
		size_t m_numActionsCompleted;
	};
}
