#pragma once

#include <RHIModule/Images/Image.h>

#include <imgui.h>

namespace Volt::RHI
{
	class Image;
}

namespace AssetBrowser
{
	class SelectionManager;
	class Item
	{
	public:
		Item(SelectionManager* selectionManager, const Filesystem::Path& path);
		virtual ~Item() = default;
		virtual bool Render();
		void StartRename();

		Filesystem::Path path;

		bool isDirectory = false;

	protected:
		float GetThumbnailSize() const;

		virtual void PushID() = 0;
		virtual IntRef<Volt::RHI::Image> GetIcon() const = 0;
		virtual ImVec4 GetBackgroundColor() const = 0;
		virtual String GetTypeName() const = 0;

		virtual void Open() {};
		virtual void SetDragDropPayload() {};
		virtual bool RenderRightClickPopup() { return false; };
		virtual bool Rename(const String& aNewName) { return false; };
		virtual void DrawAdditionalHoverInfo() {};

		void DrawHoverInfo(StringView aInfoTitle, StringView aInfo);

		SelectionManager* m_selectionManager;
		bool m_isRenaming;
		bool m_lastRenaming;
		String m_currentRenamingName;
		String m_typeName;

	private:
		glm::vec4 GetTypeNameColor(bool aHoverFlag,bool aSelectedFlag) const;

	};
}

//class DirectoryItem : public AssetBrowserItem
//{
//public:
//	~DirectoryItem() override = default;
//	void Render() override;
//
//	DirectoryItem* parentDirectory;
//
//	Vector<Ref<AssetItem>> assets;
//	Vector<Ref<DirectoryItem>> subDirectories;
//};
