#pragma once

#include "Sandbox/Window/EditorWindow.h"
#include "Sandbox/Window/MosaicEditor/MosaicNodeExtension.h"

#include <imgui_node_editor.h>

struct MosaicEditorContext
{
	ax::NodeEditor::EditorContext* editorContext = nullptr;
	ax::NodeEditor::NodeId contextNodeId;
	ax::NodeEditor::PinId contextPinId;
	ax::NodeEditor::LinkId contextLinkId;
};

namespace Volt
{
	class Texture2D;
	class MaterialAsset;
}

namespace Mosaic
{
	struct Parameter;
}

class MosaicEditorPanel : public EditorWindow
{
public:
	enum class IncompatiblePinReason
	{
		None = 0,
		IncompatibleType,
		SamePin,
	};

	MosaicEditorPanel();
	~MosaicEditorPanel() override;

	void UpdateMainContent() override;
	void UpdateContent() override;

	bool SaveSettings(const std::string& data);
	size_t LoadSettings(std::string& data);

	bool SaveNodeSettings(const UUID64 nodeId, const std::string& data);
	size_t LoadNodeSettings(const UUID64 nodeId, std::string& data);

	void OpenAsset(AssetReference<Volt::Asset> asset) override;
	void OnClose() override;

private:
	const IncompatiblePinReason CanLinkPins(const UUID64 startParam, const UUID64 endParam);

	Mosaic::Parameter& GetParameterFromID(const UUID64 paramId);

	void InitializeEditor();
	void InitializeStyle(ax::NodeEditor::Style& editorStyle);

	void DrawMenuBar();
	void DrawEditor();
	void DrawPanels();

	void DrawNodes();
	void DrawLinks();

	void DrawNodesPanel();

	void DrawContextPopups();

	void OnBeginCreate();
	void OnBeginDelete();
	void OnCopy();
	void OnPaste();

	template<typename T>
	void RegisterNodeExtension(VoltGUID nodeGUID)
	{
		VT_ENSURE(!m_nodeExtensions.contains(nodeGUID));
		m_nodeExtensions[nodeGUID] = CreateRef<T>();
	}

	MosaicEditorContext m_context;

	AssetReference<Volt::MaterialAsset> m_material;

	Map<VoltGUID, Ref<MosaicNodeExtension>> m_nodeExtensions;

	UUID64 m_newLinkPinId = 0;
	bool m_createNewNode = false;
};
