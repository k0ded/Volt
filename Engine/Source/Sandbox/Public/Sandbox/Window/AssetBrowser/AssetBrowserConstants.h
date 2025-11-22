#pragma once
#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"

#include <CoreUtilities/Allocators/PagedAtomicArenaAllocator.h>

namespace AssetBrowser
{
	typedef PagedAtomicArenaAllocator<AssetBrowser::DirectoryItem, 256> DirectoryItemAllocator;
	typedef PagedAtomicArenaAllocator<AssetBrowser::AssetItem, 1024> AssetItemAllocator;
}
