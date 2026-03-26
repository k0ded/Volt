#pragma once

#include <CoreUtilities/String/StringView.h>

enum class ActivityType
{
	Playing,
	Streaming,
	Listening,
	Watching
};

class IDiscordManager
{
public:
	virtual ~IDiscordManager() = default;

	virtual void SetApplicationID(int64_t appId) = 0;
	virtual void SetDetails(StringView text) = 0;
	virtual void SetState(StringView text) = 0;
	virtual void SetStartTime(time_t time) = 0;
	virtual void SetLargeImage(StringView imageName) = 0;
	virtual void SetLargeText(StringView text) = 0;
	virtual void SetActivityType(ActivityType type) = 0;
	virtual void SetPartySize(int32_t size) = 0;
	virtual void SetMaxPartySize(int32_t size) = 0;

	virtual void UpdateChanges() = 0;
	virtual void Update() = 0;
};
