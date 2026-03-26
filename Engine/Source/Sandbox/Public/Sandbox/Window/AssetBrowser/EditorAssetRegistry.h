#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <CoreUtilities/Containers/Vector.h>

#include <functional>

typedef std::function<Vector<std::pair<String, String>>(Volt::AssetHandle)> AssetBrowserPopupDataFunction;
//a class for registring data and open function for the asset browser
struct EditorAssetData
{
	EditorAssetData(AssetBrowserPopupDataFunction aAssetBrowserPopupDataFunction = nullptr)
		: assetBrowserPopupDataFunction(aAssetBrowserPopupDataFunction)
	{
	}
	AssetBrowserPopupDataFunction assetBrowserPopupDataFunction;
};

class EditorAssetRegistry
{
public:
	EditorAssetRegistry();
	~EditorAssetRegistry();
	
	
	static Vector<std::pair<String, String>> GetAssetBrowserPopupData(AssetType aAssetType, Volt::AssetHandle aAssetHandle);
private:
	void RegisterAssetBrowserPopupData(AssetType aAssetType, AssetBrowserPopupDataFunction aAssetBrowserPopupDataFunction);
    static std::unordered_map<AssetType, EditorAssetData> myAssetData;
};
