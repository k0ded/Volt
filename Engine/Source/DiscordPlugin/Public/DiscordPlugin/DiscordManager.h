#pragma once

#include "DiscordManagerInterface.h"

#include <LogModule/LogCategory.h>

#include <CoreUtilities/Core.h>
#include <CoreUtilities/Pointers/Unique.h>

#include <discord.h>

struct DiscordState
{
	discord::User currentUser;
	discord::Activity currentActivity;
	Unique<discord::Core> core;
};

VT_DECLARE_LOG_CATEGORY(LogDiscord, LogVerbosity::Trace);

class DiscordManager : public IDiscordManager
{
public:
	~DiscordManager() override = default;

	void SetApplicationID(int64_t appId) override;
	void SetDetails(StringView text) override;
	void SetState(StringView text) override;
	void SetStartTime(time_t time) override;
	void SetLargeImage(StringView imageName) override;
	void SetLargeText(StringView text) override;
	void SetActivityType(ActivityType type) override;
	void SetPartySize(int32_t size) override;
	void SetMaxPartySize(int32_t size) override;
	
	void UpdateChanges() override;
	void Update() override;

private:
	DiscordState m_state;
};
