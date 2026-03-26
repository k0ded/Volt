#pragma once

#include <AssetSystem/AssetType.h>
#include <AssetSystem/AssetHandle.h>

#include <CoreUtilities/Containers/Vector.h>
#include <CoreUtilities/Profiling/Profiling.h>

#include <unordered_map>
#include <typeindex>

class EditorWindow;

class EditorLibrary
{
public:
	struct PanelInfo
	{
		AssetType assetType;
		std::type_index editorType;
		String category;

		Ref<EditorWindow> editorWindow;
	};

	static void Clear();
	static void Sort();
	static Ref<EditorWindow> GetPanel(const String& panelName);

	template<typename T, typename ...Args>
	static Ref<T> RegisterWithType(const String& category, AssetType assetType, Args&& ...args);

	template<typename T, typename ...Args>
	static Ref<T> Register(const String& category, Args&& ...args);
	
	static bool OpenAsset(Volt::AssetHandle handle);
	static Ref<EditorWindow> Get(AssetType type);

	template<typename T>
	static Ref<T> Get();

	inline static const Vector<PanelInfo>& GetPanels() { return s_editors; }

private:
	inline static Vector<PanelInfo> s_editors;
};

template<typename T, typename ...Args>
inline Ref<T> EditorLibrary::RegisterWithType(const String& category, AssetType assetType, Args && ...args)
{
	VT_PROFILE_FUNCTION();

	s_editors.emplace_back(assetType, typeid(T), category, CreateRef<T>(std::forward<Args>(args)...));
	return ReinterpretRefCast<T>(s_editors.back().editorWindow);
}

template<typename T, typename ...Args>
inline Ref<T> EditorLibrary::Register(const String& category, Args && ...args)
{
	VT_PROFILE_FUNCTION();

	s_editors.emplace_back(AssetTypes::None, typeid(T), category, CreateRef<T>(std::forward<Args>(args)...));
	return ReinterpretRefCast<T>(s_editors.back().editorWindow);
}

template<typename T>
inline Ref<T> EditorLibrary::Get()
{
	auto it = std::find_if(s_editors.begin(), s_editors.end(), [&](const auto& lhs) { return lhs.editorType == typeid(T); });
	if (it == s_editors.end())
	{
		VT_LOG(Error, "Editor with type not registered!");
		return nullptr;
	}

	return ReinterpretRefCast<T>(it->editorWindow);
}
