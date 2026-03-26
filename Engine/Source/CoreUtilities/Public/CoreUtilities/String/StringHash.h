#pragma once

#include "CoreUtilities/Archive/Archive.h"

#include <cstdint>
#include <xhash>
#include <string_view>

// Enable to get and String in the StringHash to debug the actual value.
#define WITH_STRING_HASH_DEBUG 0

#if WITH_STRING_HASH_DEBUG
	#define STRING_HASH_CONSTEXPR
#else
	#define STRING_HASH_CONSTEXPR constexpr
#endif

struct StringHash
{
	STRING_HASH_CONSTEXPR StringHash()
		: hash(0)
	{ }

	STRING_HASH_CONSTEXPR StringHash(const StringHash& rhs)
		: hash(rhs.hash)
#if WITH_STRING_HASH_DEBUG
		, string(rhs.string)
#endif
	{}

	STRING_HASH_CONSTEXPR StringHash(const size_t& inHash)
		: hash(inHash)
	{}

#if WITH_STRING_HASH_DEBUG
	StringHash(const size_t& inHash, const String& inString)
		: hash(inHash), string(inString)
	{ }
#endif

	STRING_HASH_CONSTEXPR static StringHash Construct(const size_t& inHash)
	{
		return StringHash(inHash);
	}

	STRING_HASH_CONSTEXPR static StringHash Construct(StringView str)
	{
		constexpr size_t FNVOffsetBasis = 14695981039346656037ULL;
		constexpr size_t FNVPrime = 1099511628211ULL;

		size_t val = FNVOffsetBasis;

		for (char c : str)
		{
			val ^= static_cast<size_t>(c);
			val *= FNVPrime;
		}

#if WITH_STRING_HASH_DEBUG
		return StringHash(val, String(str));
#else
		return StringHash(val);
#endif
	}

	constexpr bool operator==(const StringHash& rhs) const { return hash == rhs.hash; }
	constexpr bool operator!=(const StringHash& rhs) const { return hash != rhs.hash; }
	constexpr bool operator<(const StringHash& rhs) const { return hash < rhs.hash; }
	constexpr bool operator>(const StringHash& rhs) const { return hash > rhs.hash; }
	constexpr bool operator<=(const StringHash& rhs) const { return hash < rhs.hash; }
	constexpr bool operator>=(const StringHash& rhs) const { return hash > rhs.hash; }
	constexpr StringHash& operator=(const StringHash& rhs) 
	{ 
		hash = rhs.hash;
#if WITH_STRING_HASH_DEBUG
		string = rhs.string;
#endif
		return *this;
	}

	size_t hash;

	friend Archive& operator<<(Archive& archive, StringHash& value)
	{
		archive << value.hash;
		return archive;
	}

#if WITH_STRING_HASH_DEBUG
	String string;
#endif
};

STRING_HASH_CONSTEXPR inline StringHash operator"" _sh(const char* input, size_t)
{
	return StringHash::Construct(StringView(input));
}

namespace std
{
	template<>
	struct hash<StringHash>
	{
		size_t operator()(const StringHash& hash) const
		{
			return hash.hash;
		}
	};
}
