#pragma once

#include "Sandbox/Modals/Modal.h"

#include <AssetSystem/AssetHandle.h>

#include <CoreUtilities/EnumUtils.h>

#include <functional>

CREATE_ENUM(RequiredSaveAction,
	None,
	Create,
	CheckOut
);

class CheckoutFilesModal final : public Modal
{
public:
	CheckoutFilesModal(const std::string& strId);
	~CheckoutFilesModal() override = default;

	static bool NeedsAction(Volt::AssetHandle handle);
	static RequiredSaveAction GetRequiredAction(Volt::AssetHandle handle);

	template<typename Allocator>
	void SetAssetsToHandle(Vector<Volt::AssetHandle, Allocator> assets)
	{
		m_assetsToHandle.resize_uninitialized(assets.size());
		FillRequiredActionsMap();
		memcpy_s(m_assetsToHandle.data(), m_assetsToHandle.byte_size() * sizeof(Volt::AssetHandle), assets.data(), assets.byte_size());
		Sort();
	}

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	void FillRequiredActionsMap();
	enum class TableColumns
	{
		Selected,
		Status,
		Name,
		Type,
		Path,
		NUM
	};
	

	std::string GetTableColumnName(TableColumns tableColumn);

	void DrawRowColumn(int32_t index, TableColumns tableColumn);

	void Sort();
	bool m_SortingAscending = false;

	Vector<Volt::AssetHandle> m_assetsToHandle;
	std::set<Volt::AssetHandle> m_selectedAssets;
	Map<Volt::AssetHandle, RequiredSaveAction> m_assetToRequiredAction;

};
