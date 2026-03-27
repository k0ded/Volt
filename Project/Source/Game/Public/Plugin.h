#pragma once

#include <Volt-Core/Plugin/Plugin.h>

class PLUGIN_API GamePlugin : public Volt::Plugin
{
public:
	~GamePlugin() override = default;

	VT_DECLARE_PLUGIN_GUID("{F2F99642-8530-494C-997B-4101B0FCEEBF}"_guid);

	inline uint32_t GetVersion() const override { return 1; }
	inline StringView GetName() const override { return "GamePlugin"; }
	inline StringView GetDescription() const override { return "None"; }
	inline StringView GetCategory() const override { return "None"; }

	void Initialize() override;
	void Shutdown() override;

private:
};

VT_REGISTER_PLUGIN(GamePlugin);