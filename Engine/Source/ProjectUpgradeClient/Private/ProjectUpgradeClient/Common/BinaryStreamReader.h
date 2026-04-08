#pragma once

#include "ProjectUpgradeClient/Common/StreamCommon.h"

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Filesystem/Path.h>

#include <CoreModule/DataBuffer.h>

#include <fstream>

#include <array>
#include <string>
#include <map>
#include <unordered_map>

class BinaryStreamReader
{
public:
	BinaryStreamReader(const Filesystem::Path& filePath);
	BinaryStreamReader(const Filesystem::Path& filePath, const size_t maxLoadSize);

	bool IsStreamValid() const;

	template<typename T>
	void Read(T& outData);

	template<>
	void Read(String& data);

	template<>
	void Read(Filesystem::Path& data);

	template<>
	void Read(DataBuffer& data);

	template<typename T>
	bool TryRead(T& outData);

	template<typename F>
	void Read(Vector<F>& data);

	template<typename F, size_t NumValues>
	void Read(Vector<F, InlineAllocator<NumValues>>& data);

	template<typename F>
	void ReadRaw(Vector<F>& data);

	template<typename F, size_t COUNT>
	void Read(std::array<F, COUNT>& data);

	template<typename Key, typename Value>
	void Read(std::map<Key, Value>& data);

	template<typename Key, typename Value>
	void Read(std::unordered_map<Key, Value>& data);

	template<typename Key, typename Value>
	void Read(Map<Key, Value>& data);

	void Read(void* data);
	
	//note: this will override any data in the destination vector
	template<typename AllocatorType = DefaultHeapAllocator>
	void ReadBytesRaw(Vector<uint8_t, AllocatorType>& destination, size_t numBytes);

	void ResetHead();
	TypeHeader ReadTypeHeader();

	size_t GetRemainingDataSize();

	bool IsAtEnd();

private:
	void ReadData(void* outData, const TypeHeader& serializedTypeHeader, const TypeHeader& constructedTypeHeader);

	bool Decompress(size_t compressedDataOffset);

	Vector<uint8_t> m_data;
	size_t m_currentOffset = 0;
	bool m_streamValid = false;
	bool m_compressed = false;
};

template<typename T>
inline void BinaryStreamReader::Read(T& outData)
{
	constexpr size_t typeSize = sizeof(T);

	TypeHeader typeHeader{};
	typeHeader.totalTypeSize = static_cast<uint32_t>(typeSize);

	TypeHeader serializedTypeHeader = ReadTypeHeader();

	if constexpr (std::is_trivial_v<T>)
	{
		ReadData(&outData, serializedTypeHeader, typeHeader);
	}
	else
	{
		Deserialize(*this, outData);
	}
}

template<typename T>
inline bool BinaryStreamReader::TryRead(T& outData)
{
	constexpr size_t typeSize = sizeof(T);

	TypeHeader typeHeader{};
	typeHeader.totalTypeSize = static_cast<uint32_t>(typeSize);

	TypeHeader serializedTypeHeader = ReadTypeHeader();

	if (serializedTypeHeader.totalTypeSize != typeHeader.totalTypeSize)
	{
		return false;
	}

	if constexpr (std::is_trivial_v<T>)
	{
		ReadData(&outData, serializedTypeHeader, typeHeader);
	}
	else
	{
		Deserialize(*this, outData);
	}

	return true;
}

template<>
inline void BinaryStreamReader::Read(String& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	data.resize(serializedTypeHeader.totalTypeSize);
	if (serializedTypeHeader.totalTypeSize > 0)
	{
		ReadData(data.data(), serializedTypeHeader, typeHeader);
	}
}

template<>
inline void BinaryStreamReader::Read(Filesystem::Path& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	String filepathStr;
	filepathStr.resize(serializedTypeHeader.totalTypeSize);
	if (serializedTypeHeader.totalTypeSize > 0)
	{
		ReadData(filepathStr.data(), serializedTypeHeader, typeHeader);
	}

	data = filepathStr;
}

template<>
inline void BinaryStreamReader::Read(DataBuffer& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	data.Resize(serializedTypeHeader.totalTypeSize);
	if (serializedTypeHeader.totalTypeSize > 0)
	{
		ReadData(data.As<void>(), serializedTypeHeader, typeHeader);
	}
}

template<typename F>
inline void BinaryStreamReader::Read(Vector<F>& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	data.resize(serializedTypeHeader.totalTypeSize);

	if constexpr (std::is_trivial_v<F>)
	{
		// We must multiply type size to get correct byte size
		serializedTypeHeader.totalTypeSize *= sizeof(F);
		ReadData(data.data(), serializedTypeHeader, typeHeader);
	}
	else
	{
		for (size_t i = 0; i < data.size(); i++)
		{
			Read(data[i]);
		}
	}
}

template<typename F, size_t NumValues>
inline void BinaryStreamReader::Read(Vector<F, InlineAllocator<NumValues>>& data)
{
	size_t serializedNumValues;
	Read(serializedNumValues);

	TypeHeader typeHeader{};
	typeHeader.totalTypeSize = static_cast<uint32_t>(NumValues * sizeof(F));

	TypeHeader serializedTypeHeader = ReadTypeHeader();

	//call reserve here to make sure the begin ptr has been assigned
	data.reserve(NumValues);
	data.resize(serializedNumValues);
	memset(data.data(), 0, NumValues);

	ReadData(data.data(), serializedTypeHeader, typeHeader);
}

template<typename F>
inline void BinaryStreamReader::ReadRaw(Vector<F>& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	data.resize(serializedTypeHeader.totalTypeSize);
	serializedTypeHeader.totalTypeSize *= sizeof(F);
	ReadData(data.data(), serializedTypeHeader, typeHeader);
}

template<typename F, size_t COUNT>
inline void BinaryStreamReader::Read(std::array<F, COUNT>& data)
{
	TypeHeader typeHeader{};
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	if constexpr (std::is_trivial_v<F>)
	{
		// We must multiply type size to get correct byte size
		serializedTypeHeader.totalTypeSize *= sizeof(F);
		ReadData(data.data(), serializedTypeHeader, typeHeader);
	}
	else
	{
		for (size_t i = 0; i < data.size(); i++)
		{
			Read(data[i]);
		}
	}
}

template<typename Key, typename Value>
inline void BinaryStreamReader::Read(std::map<Key, Value>& data)
{
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	const size_t elementCount = serializedTypeHeader.totalTypeSize;

	for (size_t i = 0; i < elementCount; i++)
	{
		Key key{};

		if constexpr (std::is_trivial_v<Key>)
		{
			TypeHeader keyTypeHeader{};
			keyTypeHeader.totalTypeSize = sizeof(Key);

			ReadData(&key, keyTypeHeader, keyTypeHeader);
		}
		else
		{
			Key::Deserialize(*this, key);
		}

		Value value{};

		if constexpr (std::is_trivial_v<Value>)
		{
			TypeHeader valueTypeHeader{};
			valueTypeHeader.totalTypeSize = sizeof(Value);

			TypeHeader valueSerializedTypeHeader = ReadTypeHeader();
			ReadData(&value, valueSerializedTypeHeader, valueTypeHeader);
		}
		else
		{
			Value::Deserialize(*this, value);
		}

		data.emplace(key, value);
	}
}

template<typename Key, typename Value>
inline void BinaryStreamReader::Read(std::unordered_map<Key, Value>& data)
{
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	const size_t elementCount = serializedTypeHeader.totalTypeSize;

	for (size_t i = 0; i < elementCount; i++)
	{
		Key key{};

		if constexpr (std::is_trivial_v<Key>)
		{
			TypeHeader keyTypeHeader{};
			keyTypeHeader.totalTypeSize = sizeof(Key);

			ReadData(&key, keyTypeHeader, keyTypeHeader);
		}
		else if constexpr (std::is_same<Key, String>::value)
		{
			Read(key);
		}
		else
		{
			Key::Deserialize(*this, key);
		}
		 
		Value value{};

		if constexpr (std::is_trivial_v<Value>)
		{
			TypeHeader valueTypeHeader{};
			valueTypeHeader.totalTypeSize = sizeof(Value);

			ReadData(&value, valueTypeHeader, valueTypeHeader);
		}
		else if constexpr (std::is_same<Value, String>::value)
		{
			Read(value);
		}
		else
		{
			Value::Deserialize(*this, value);
		}

		data[key] = value;
	}
}

template<typename Key, typename Value>
inline void BinaryStreamReader::Read(Map<Key, Value>& data)
{
	TypeHeader serializedTypeHeader = ReadTypeHeader();

	const size_t elementCount = serializedTypeHeader.totalTypeSize;

	for (size_t i = 0; i < elementCount; i++)
	{
		Key key{};

		if constexpr (std::is_trivial_v<Key>)
		{
			TypeHeader keyTypeHeader{};
			keyTypeHeader.totalTypeSize = sizeof(Key);

			ReadData(&key, keyTypeHeader, keyTypeHeader);
		}
		else if constexpr (std::is_same<Key, String>::value)
		{
			Read(key);
		}
		else
		{
			Key::Deserialize(*this, key);
		}

		Value value{};

		if constexpr (std::is_trivial_v<Value>)
		{
			TypeHeader valueTypeHeader{};
			valueTypeHeader.totalTypeSize = sizeof(Value);

			ReadData(&value, valueTypeHeader, valueTypeHeader);
		}
		else if constexpr (std::is_same<Value, String>::value)
		{
			Read(value);
		}
		else
		{
			Value::Deserialize(*this, value);
		}

		data[key] = value;
	}
}

template<typename AllocatorType>
inline void BinaryStreamReader::ReadBytesRaw(Vector<uint8_t, AllocatorType>& destination, size_t numBytes)
{
	VT_ASSERT(numBytes <= GetRemainingDataSize());

	destination.resize_uninitialized(numBytes);
	memcpy_s(destination.data(), destination.size(), &m_data[m_currentOffset], numBytes);

	m_currentOffset += numBytes;
}
