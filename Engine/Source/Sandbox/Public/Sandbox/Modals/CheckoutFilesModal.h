#pragma once

#include "Sandbox/Modals/Modal.h"

#include <functional>

class CheckoutFilesModal final : public Modal
{
public:
	CheckoutFilesModal(const std::string& strId);
	~CheckoutFilesModal() override = default;

	void SetAssetsToCheckout(Vector<std::filesystem::path> filePaths);
	void SetOnConfirm(std::function<void()> onConfirm);
	void SetOnCancel(std::function<void()> onCancel);

protected:
	void DrawModalContent() override;
	void OnOpen() override;
	void OnClose() override;

private:
	enum class TableColumns
	{
		Selected,
		Name,
		Type,
		Path
	};

	std::string GetTableColumnName(TableColumns tableColumn);

	void DrawHeaderForColumn(TableColumns tableColumn);
	void DrawRowColumn(int32_t index, TableColumns tableColumn);

	void Sort();
	bool m_SortingAscending = false;

	std::function<void()> m_onConfirm;
	std::function<void()> m_onCancel;


	Vector<std::filesystem::path> m_FilePaths;
	std::set<int32_t> m_SelectedIndices;
	Vector<int32_t> m_SortedIndices;
};
