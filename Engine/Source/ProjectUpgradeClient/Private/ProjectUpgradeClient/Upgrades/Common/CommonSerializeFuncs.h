#pragma once

#include "ProjectUpgradeClient/Upgrades/Common/BinaryStreamWriter.h"
#include "ProjectUpgradeClient/Upgrades/Common/BinaryStreamReader.h"

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/VoltGUID.h>

static void Serialize(BinaryStreamWriter& streamWriter, const UUID32& data)
{
	streamWriter.Write(*reinterpret_cast<const uint32_t*>(&data));
}

static void Deserialize(BinaryStreamReader& streamReader, UUID32& outData)
{
	streamReader.Read(*reinterpret_cast<uint32_t*>(&outData));
}

static void Serialize(BinaryStreamWriter& streamWriter, const UUID64& data)
{
	streamWriter.Write(*reinterpret_cast<const uint64_t*>(&data));
}

static void Deserialize(BinaryStreamReader& streamReader, UUID64& outData)
{
	streamReader.Read(*reinterpret_cast<uint64_t*>(&outData));
}

static void Serialize(BinaryStreamWriter& streamWriter, const VoltGUID& data)
{
	streamWriter.Write(data.loPart);
	streamWriter.Write(data.hiPart);
}

static void Deserialize(BinaryStreamReader& streamReader, VoltGUID& outData)
{
	streamReader.Read(outData.loPart);
	streamReader.Read(outData.hiPart);
}

