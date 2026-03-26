#include "cupch.h"
#include "CoreUtilities/Archive/Archive.h"
#include "CoreUtilities/Archive/ArchiveVersionRegistry.h"

Archive::Archive(bool isLoading)
	: m_isLoading(isLoading)
{

}

Archive::Archive(Archive&& other) noexcept
	: m_isLoading(other.m_isLoading),
	m_versions(std::move(other.m_versions))
{}

Archive::Archive(const Archive& other)
	: m_isLoading(other.m_isLoading),
	m_versions(other.m_versions)
{

}

Archive& Archive::operator=(const Archive& other)
{
	if (&other != this)
	{
		m_versions = other.m_versions;
	}

	return *this;
}

Archive& Archive::operator=(Archive&& other) noexcept
{
	if (&other != this)
	{
		m_versions = std::move(other.m_versions);
	}

	return *this;
}

void Archive::UseVersion(const VoltGUID& guid)
{
	if (m_isLoading)
	{
		return;
	}

	VT_ENSURE_MSG(ArchiveVersionRegistry::Get().IsVersionRegistered(guid), "Version is not registered!");

	auto it = std::find_if(m_versions.begin(), m_versions.end(), [&guid](const VersionInfo& versionInfo)
	{
		return versionInfo.guid == guid;
	});

	if (it == m_versions.end())
	{
		m_versions.emplace_back(guid);
	}
}

int32_t Archive::GetVersion(const VoltGUID& guid) const
{
	if (!m_isLoading)
	{
		return -1;
	}

	auto it = std::find_if(m_versions.begin(), m_versions.end(), [&guid](const VersionInfo& versionInfo)
	{
		return versionInfo.guid == guid;
	});

	if (it != m_versions.end())
	{
		return it->version;
	}

	return -1;
}

Archive& operator<<(Archive& archive, String& value)
{
	size_t size = value.size();
	archive << size;

	if (archive.IsLoading())
	{
		value.resize(size);
	}

	if (size > 0)
	{
		archive.SerializeBytes(value.data(), value.size());
	}
	return archive;
}

Archive& operator<<(Archive& archive, Filesystem::Path& value)
{
	String tempString = value.ToString();
	archive << tempString;

	if (archive.IsLoading())
	{
		value = tempString;
	}

	return archive;
}

Archive& operator<<(Archive& archive, VoltGUID& value)
{
	archive << value.loPart;
	archive << value.hiPart;
	return archive;
}

Archive& operator<<(Archive& archive, Archive& value)
{
	if (&archive != &value)
	{
		value.Serialize(archive);
	}
	return archive;
}

Archive& operator<<(Archive& archive, UUID64& value)
{
	uint64_t tempValue = value.Get();
	archive << tempValue;

	if (archive.IsLoading())
	{
		value = { tempValue };
	}
	return archive;
}

Archive& operator<<(Archive& archive, UUID32& value)
{
	uint32_t tempValue = value.Get();
	archive << tempValue;

	if (archive.IsLoading())
	{
		value = { tempValue };
	}
	return archive;
}

Archive& operator<<(Archive& archive, Archive::VersionInfo& value)
{
	archive << value.guid;
	archive << value.version;

	return archive;
}
