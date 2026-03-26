#pragma once

#include "Sandbox/Modals/Modal.h"

#include <AssetSystem/AssetHandle.h>

#include <CoreUtilities/Containers/VectorVariants.h>
#include <CoreUtilities/EnumUtils.h>
#include <CoreUtilities/Containers/Map.h>
#include <CoreUtilities/Filesystem/Path.h>


CREATE_ENUM(CreateFilesTableColumns,
	Selected,
	Status,

	Name,
	Type,
	Path,
	AssetHandle,

	SetPath,

	NUM
);

enum class AssetModalType
{
	Create,
	Save,
	SaveOrDiscard,
	CheckOut,
	None
};

enum class AssetModalResult
{
	Cancel,
	MakeWriteable,
	CheckOut,
	Save,
	Create,
	Discard,
	None
};

class AssetsModal final : public Modal
{
public:
	AssetsModal(const String& strId);
	~AssetsModal() override = default;

	//this should be called instead of OpenModalBlocking
	template<typename Allocator>
	[[nodiscard]] AssetModalResult OpenAssetModalTypeBlocking(AssetModalType inAssetModalType,
		const Vector<Volt::AssetHandle,
		Allocator>& inAssets,
		std::set<Volt::AssetHandle>& outSelectedAssets,
		const Map<Volt::AssetHandle, String /*disabled reason*/>* disabledAssets = nullptr)
	{
		m_assetHandles.resize_uninitialized(inAssets.size());
		memcpy_s(m_assetHandles.data(), m_assetHandles.byte_size() * sizeof(Volt::AssetHandle), inAssets.data(), inAssets.byte_size());
		return OpenAssetModalTypeBlockingImpl(inAssetModalType, outSelectedAssets, disabledAssets);
	}

	[[nodiscard]] Filesystem::Path GetNewAssetPath(Volt::AssetHandle handle) 
	{
		VT_ASSERT(m_assetToNewPath.contains(handle));
		return m_assetToNewPath[handle];
	}
	
private:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;
private:
	AssetModalResult OpenAssetModalTypeBlockingImpl(AssetModalType inAssetModalType,
		std::set<Volt::AssetHandle>& outSelectedAssets,
		const Map<Volt::AssetHandle, String /*disabled reason*/>* disabledAssets);
	AssetModalResult GetResult() { return m_result; }
	bool ShouldColumnExist(CreateFilesTableColumns column);

	void SetupTableColumns();
	void SetupTableColumn(CreateFilesTableColumns column);

	void DrawHeaderRowColumn(CreateFilesTableColumns column);
	void DrawRowColumn(CreateFilesTableColumns column ,Volt::AssetHandle handle);
	void DrawDecisionButtons();

	static constexpr glm::vec4 PrimaryButtonColor = { 0.008f, 0.243f, 0.541f, 1.f };

	AssetModalType m_assetModalType;
	AssetModalResult m_result;

	uint32_t m_numColumns;

	Vector<Volt::AssetHandle> m_assetHandles;
	std::set<Volt::AssetHandle> m_selectedAssets;
	Map<Volt::AssetHandle, Filesystem::Path> m_assetToNewPath;
	Map<Volt::AssetHandle, String> m_disabledAssets;

};
