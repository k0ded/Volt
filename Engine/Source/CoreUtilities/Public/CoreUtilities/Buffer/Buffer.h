#pragma once

#include "CoreUtilities/Core.h"
#include "CoreUtilities/Containers/Vector.h"

#include <fstream>
#include <cassert>
#include <filesystem>

class VTCOREUTIL_API Buffer
{
public:
	inline Buffer(size_t aSize);
	inline Buffer() = default;
	inline ~Buffer();

	inline void Release();
	inline void Allocate(size_t aSize);
	inline void Clear();
	inline void Resize(size_t aSize);
	inline void Copy(const void* aSrcData, size_t aSize, size_t aOffset = 0);

	inline const bool IsValid() const;
	inline const size_t GetSize() const;

	inline static bool WriteToFile(Buffer buffer, const std::filesystem::path& targetPath);
	inline static Buffer ReadFromFile(const std::filesystem::path& targetPath);

	template<typename T>
	inline T* As(size_t offset = 0) const;

private:
	Ref<uint8_t[]> m_data = nullptr;
	size_t m_size = 0;
};

inline Buffer::Buffer(size_t aSize)
{
	Allocate(aSize);
}

inline Buffer::~Buffer()
{
}

inline void Buffer::Release()
{
	m_data.reset();
	m_size = 0;
}

inline void Buffer::Allocate(size_t aSize)
{
	if (aSize == 0)
	{
		return;
	}

	Release();
	m_data = Ref<uint8_t[]>(new uint8_t[aSize]{0}, [](uint8_t* p)
	{
		delete[] p;
	});

	m_size = aSize;
}

inline void Buffer::Clear()
{
	memset(m_data.get(), 0, m_size);
}

inline void Buffer::Resize(size_t aSize)
{
	if (m_size < aSize)
	{
		Ref<uint8_t[]> newBuffer = Ref<uint8_t[]>(new uint8_t[aSize]{ 0 }, [](uint8_t* p)
		{
			delete[] p;
		});

		if (m_data)
		{
			memcpy_s(newBuffer.get(), aSize, m_data.get(), m_size);
		}

		m_size = aSize;
		m_data = newBuffer;
	}
}

inline void Buffer::Copy(const void* aSrcData, size_t aSize, size_t aOffset)
{
	if (aSize == 0)
	{
		return;
	}

	assert(aOffset + aSize <= m_size && "Cannot copy into buffer of lesser size!");
	memcpy_s(m_data.get() + aOffset, m_size, aSrcData, aSize);
}

inline const bool Buffer::IsValid() const
{
	return m_data != nullptr;
}

inline const size_t Buffer::GetSize() const
{
	return m_size;
}

inline bool Buffer::WriteToFile(Buffer buffer, const std::filesystem::path& targetPath)
{
	std::ofstream file(targetPath, std::ios::out | std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	file.write(reinterpret_cast<char*>(buffer.m_data.get()), buffer.m_size);
	file.close();

	return true;
}

inline Buffer Buffer::ReadFromFile(const std::filesystem::path& targetPath)
{
	if (!std::filesystem::exists(targetPath))
	{
		return {};
	}

	std::ifstream file(targetPath, std::ios::in | std::ios::binary);
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

	Buffer buffer{ srcSize };
	buffer.Copy(totalData.data(), totalData.size());

	return buffer;
}

template<typename T>
inline T* Buffer::As(size_t offset) const
{
	return reinterpret_cast<T*>(m_data.get() + offset);
}
