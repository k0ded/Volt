#pragma once

#include <AssetSystem/AssetHandle.h>

struct GlobalEditorStates
{
	inline static bool isDragging = false;
	inline static bool dragStartedInAssetBrowser = false;
	inline static Volt::AssetHandle dragAsset = Volt::Asset_New::Null();
};
