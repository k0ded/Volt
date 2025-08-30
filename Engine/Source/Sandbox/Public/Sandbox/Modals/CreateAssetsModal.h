#pragma once

#include "Sandbox/Modals/Modal.h"

#include <AssetSystem/AssetHandle.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/EnumUtils.h>

#include <filesystem>

CREATE_ENUM(CreateFilesTableColumns,
	Status,
	Name,
	Type,
	Path,
	SetPath,
	NUM
);

class CreateFilesModal final : public Modal
{
public:
	CreateFilesModal(const std::string& strId);
	~CreateFilesModal() override = default;

	static bool NeedsAction(Volt::AssetHandle handle);

	template<typename Allocator>
	void SetAssetsToHandle(Vector<Volt::AssetHandle, Allocator> assets)
	{
		m_assetHandles.resize_uninitialized(assets.size());
		memcpy_s(m_assetHandles.data(), m_assetHandles.byte_size() * sizeof(Volt::AssetHandle), assets.data(), assets.byte_size());
		//Sort();
	}

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	void DrawRowColumn(CreateFilesTableColumns column ,Volt::AssetHandle handle);

	Vector<Volt::AssetHandle> m_assetHandles;
	std::set<Volt::AssetHandle> m_selectedAssets;
	Map<Volt::AssetHandle, std::filesystem::path> m_assetToNewPath;

};
