#pragma once

#include "CoreModule/Config.h"

#include <CoreUtilities/String/VoltString.h>

namespace Volt
{
	class Version
	{
	public:
		Version() = default;
		VTC_API Version(const String& strValue);
		VTC_API Version(const uint32_t major, const uint32_t minor, const uint32_t patch);

		VTC_API static Version CreateFromString(const String& strValue);
		VTC_API static Version Create(const uint32_t major, const uint32_t minor, const uint32_t patch);

		VT_NODISCARD VTC_API const String ToString() const;
		VT_NODISCARD VTC_API const bool IsValid() const;

		VT_NODISCARD inline const uint32_t GetMajor() const { return m_major; }
		VT_NODISCARD inline const uint32_t GetMinor() const { return m_minor; }
		VT_NODISCARD inline const uint32_t GetPatch() const { return m_patch; }

		friend inline bool operator==(const Version& lhs, const Version& rhs)
		{
			return lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor && lhs.m_patch == rhs.m_patch;
		}

		friend inline bool operator!=(const Version& lhs, const Version& rhs)
		{
			return !(lhs == rhs);
		}

		friend inline bool operator<(const Version& lhs, const Version& rhs)
		{
			return lhs.m_major < rhs.m_major || (lhs.m_major == rhs.m_major && lhs.m_minor < rhs.m_major) || (lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor && lhs.m_patch < rhs.m_patch);
		}

		friend inline bool operator> (const Version& lhs, const Version& rhs) { return rhs < lhs; }
		friend inline bool operator<=(const Version& lhs, const Version& rhs) { return !(lhs > rhs); }
		friend inline bool operator>=(const Version& lhs, const Version& rhs) { return !(lhs < rhs); }

	private:
		uint32_t m_major = 0;
		uint32_t m_minor = 0;
		uint32_t m_patch = 0;
	};
}

namespace std
{
	template<>
	struct hash<Volt::Version>
	{
		size_t operator()(const Volt::Version& version) const
		{
			std::hash<uint64_t> hasher;
			return hasher(version.GetMajor()) ^ hasher(version.GetMinor()) ^ hasher(version.GetPatch());
		}
	};
}
