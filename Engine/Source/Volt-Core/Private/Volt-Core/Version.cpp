#include "vtcorepch.h"
#include "Volt-Core/Version.h"

#include <CoreUtilities/String/StringUtility.h>

namespace Volt
{
	Version::Version(const String& strValue)
	{
		Vector<String> strParts = Utility::SplitStringsByCharacter(strValue, '.');
		if (strParts.size() != 3)
		{
			return;
		}

		m_major = StoI(strParts[0]);
		m_minor = StoI(strParts[1]);
		m_patch = StoI(strParts[2]);
	}

	Version::Version(const uint32_t major, const uint32_t minor, const uint32_t patch)
		: m_major(major), m_minor(minor), m_patch(patch)
	{
	}

	Version Version::CreateFromString(const String& strValue)
	{
		return Version(strValue);
	}

	Version Version::Create(const uint32_t major, const uint32_t minor, const uint32_t patch)
	{
		return Version(major, minor, patch);
	}

	const String Version::ToString() const
	{
		return FormatString("{}.{}.{}", m_major, m_minor, m_patch);
	}

	const bool Version::IsValid() const
	{
		return m_major != 0 || m_minor != 0 || m_patch != 0;
	}
}
