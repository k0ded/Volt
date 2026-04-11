#pragma once

#include <CoreUtilities/Archive/Archive.h>
#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Pointers/Ref.h>

class DataBuffer
{
public:
	inline DataBuffer(size_t aSize);
	inline DataBuffer() = default;
	inline ~DataBuffer();

	inline void Release();
	inline void Allocate(size_t aSize);
	inline void Clear();
	inline void Resize(size_t aSize);
	inline void Copy(const void* aSrcData, size_t aSize, size_t aOffset = 0);

	inline const bool IsValid() const;
	inline const size_t GetSize() const;

	template<typename T>
	inline T* As(size_t offset = 0) const;

private:
	Ref<uint8_t[]> m_data = nullptr;
	size_t m_size = 0;
};

inline DataBuffer::DataBuffer(size_t aSize)
{
	Allocate(aSize);
}

inline DataBuffer::~DataBuffer()
{
}

inline void DataBuffer::Release()
{
	m_data.Reset();
	m_size = 0;
}

inline void DataBuffer::Allocate(size_t aSize)
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

inline void DataBuffer::Clear()
{
	memset(m_data.GetRaw(), 0, m_size);
}

inline void DataBuffer::Resize(size_t aSize)
{
	if (m_size < aSize)
	{
		Ref<uint8_t[]> newBuffer = Ref<uint8_t[]>(new uint8_t[aSize]{ 0 }, [](uint8_t* p)
		{
			delete[] p;
		});

		if (m_data)
		{
			memcpy_s(newBuffer.GetRaw(), aSize, m_data.GetRaw(), m_size);
		}

		m_size = aSize;
		m_data = newBuffer;
	}
}

inline void DataBuffer::Copy(const void* aSrcData, size_t aSize, size_t aOffset)
{
	if (aSize == 0)
	{
		return;
	}

	VT_ASSERT(aOffset + aSize <= m_size && "Cannot copy into buffer of lesser size!");
	memcpy_s(m_data.GetRaw() + aOffset, m_size, aSrcData, aSize);
}

inline const bool DataBuffer::IsValid() const
{
	return m_data != nullptr;
}

inline const size_t DataBuffer::GetSize() const
{
	return m_size;
}

template<typename T>
inline T* DataBuffer::As(size_t offset) const
{
	return reinterpret_cast<T*>(m_data.GetRaw() + offset);
}

inline Archive& operator<<(Archive& archive, DataBuffer& buffer)
{
	size_t size = buffer.GetSize();
	archive << size;

	if (archive.IsLoading())
	{
		buffer.Allocate(size);
	}

	if (size > 0)
	{
		archive.SerializeBytes(buffer.As<void*>(), size);
	}
	return archive;
}
