#pragma once
#include "Sandbox/Window/AssetBrowser/DirectoryItem.h"
#include "Sandbox/Window/AssetBrowser/AssetItem.h"

#include <CoreUtilities/Allocators/PagedArenaAllocator.h>

namespace AssetBrowser
{
	typedef PagedArenaAllocator<AssetBrowser::DirectoryItem, 256> DirectoryItemAllocator;
	typedef PagedArenaAllocator<AssetBrowser::AssetItem, 1024> AssetItemAllocator;
}
