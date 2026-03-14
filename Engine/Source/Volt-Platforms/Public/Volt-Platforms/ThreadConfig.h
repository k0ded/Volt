#pragma once

namespace Volt
{
	struct ThreadConfig
	{
		bool isWorkerThread;
		bool isIOThread;
		bool isMainThread;
	};
}
