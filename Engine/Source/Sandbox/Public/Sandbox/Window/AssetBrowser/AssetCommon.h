#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

struct AssetData
{
	Volt::AssetHandle handle = 0;
	AssetType type = AssetTypes::None;
	Filesystem::Path path;
	bool selected = false;
};

struct DirectoryData
{
	Volt::AssetHandle handle;
	Filesystem::Path path;

	DirectoryData* parentDir;
	bool selected = false;

	Vector<AssetData> assets;
	Vector<Ref<DirectoryData>> subDirectories;
};
