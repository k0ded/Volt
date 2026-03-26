#pragma once

template<typename T, typename Allocator>
Archive& operator<<(Archive& archive, Vector<T, Allocator>& value)
{
	size_t size = value.size();
	archive << size;

	if (archive.IsLoading())
	{
		value.resize(size);
	}

	for (size_t i = 0; i < size; ++i)
	{
		archive << value[i];
	}
	return archive;
}

template<Pod T, typename Allocator>
Archive& operator<<(Archive& archive, Vector<T, Allocator>& value)
{
	size_t size = value.size();
	archive << size;

	if (archive.IsLoading())
	{
		value.resize_uninitialized(size);
	}

	if (size > 0)
	{
		archive.SerializeBytes(value.data(), value.byte_size());
	}
	return archive;
}

template<typename T, size_t Count>
Archive& operator<<(Archive& archive, Array<T, Count>& value)
{
	for (size_t i = 0; i < value.size(); ++i)
	{
		archive << value[i];
	}
	return archive;
}

template<Pod T, size_t Count>
Archive& operator<<(Archive& archive, Array<T, Count>& value)
{
	archive.SerializeBytes(value.data(), value.byte_size());
	return archive;
}

template<typename Key, typename Value>
Archive& operator<<(Archive& archive, Map<Key, Value>& map)
{
	size_t size = map.size();
	archive << size;

	if (archive.IsLoading())
	{
		map.reserve(size);

		for (size_t i = 0; i < size; ++i)
		{
			Key k;
			Value v;

			archive << k;
			archive << v;

			map[k] = std::move(v);
		}
	}
	else
	{
		for (auto& [key, v] : map)
		{
			archive << key;
			archive << v;
		}
	}

	return archive;
}

template<Arithmetic T>
Archive& operator<<(Archive& archive, T& value)
{
	archive.SerializeBytes(&value, sizeof(T));
	return archive;
}

template<MathType T>
Archive& operator<<(Archive& archive, T& value)
{
	archive.SerializeBytes(&value, sizeof(T));
	return archive;
}

template<Enum T>
Archive& operator<<(Archive& archive, T& value)
{
	using UnderlyingType = std::underlying_type_t<T>;
	UnderlyingType& tempValue = *reinterpret_cast<UnderlyingType*>(&value);
	archive << tempValue;
	return archive;
}

template<typename T, typename V>
Archive& operator<<(Archive& archive, std::pair<T, V>& value)
{
	archive << value.first;
	archive << value.second;

	return archive;
}
