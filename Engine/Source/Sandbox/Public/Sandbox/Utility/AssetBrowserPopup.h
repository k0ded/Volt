#pragma once

#include <AssetSystem/AssetHandle.h>
#include <AssetSystem/AssetType.h>

#include <CoreUtilities/Containers/Vector.h>

class AssetBrowserPopup
{
public:
	enum class State
	{
		Changed,
		Closed,
		Open
	};

	AssetBrowserPopup(const String& id, AssetType wantedType, Volt::AssetHandle& handler);

	State Update();

private:
	State RenderView(const Vector<Volt::AssetHandle>& items);

	String myId;
	AssetType myWantedType;
	Volt::AssetHandle& myHandle;

	String mySearchQuery;
	bool myActivateSearch = false;
};
