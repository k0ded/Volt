#include "ProjectUpgradeClient/Upgrades/0_1_7/AssetSerializationCommon.h"
#include "ProjectUpgradeClient/Upgrades/Common/CommonSerializeFuncs.h"

namespace Volt
{
	void Serialize(BinaryStreamWriter& streamWriter, const SerializedAssetMetadata& data)
	{
		streamWriter.Write(data.handle);
		streamWriter.Write(data.type->GetGUID());
		streamWriter.Write(data.version);
		streamWriter.Write(data.customData);
	}

	void Deserialize(BinaryStreamReader& streamReader, SerializedAssetMetadata& outData)
	{
		VoltGUID guid{};

		streamReader.Read(outData.handle);
		streamReader.Read(guid);
		streamReader.Read(outData.version);
		streamReader.Read(outData.customData);
		outData.type = AssetTypeRegistry::Get().GetTypeFromGUID(guid);
	}
}
