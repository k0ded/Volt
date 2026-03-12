#pragma once

#include "ProjectUpgradeClient/Common/BinaryStreamWriter.h"
#include "ProjectUpgradeClient/Common/BinaryStreamReader.h"

#include <EntitySystem/EntityID.h>

#include <CoreUtilities/UUID.h>
#include <CoreUtilities/VoltGUID.h>

#include <yaml-cpp/yaml.h>

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

inline YAML::Emitter& operator<<(YAML::Emitter& out, const Volt::AssetHandle& handle)
{
	out << static_cast<uint64_t>(handle);
	return out;
}

namespace YAML
{
	template<>
	struct convert<Volt::EntityID>
	{
		static Node encode(const Volt::EntityID& rhs)
		{
			Node node;
			node.push_back((uint32_t)rhs);
			return node;
		};

		static bool decode(const Node& node, Volt::EntityID& v)
		{
			v = node.as<uint32_t>();
			return true;
		};
	};
}
