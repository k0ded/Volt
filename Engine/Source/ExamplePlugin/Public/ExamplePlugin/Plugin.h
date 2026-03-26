#pragma once

#include <Volt-Core/Plugin/Plugin.h>

class PLUGIN_API ExamplePlugin : public Volt::Plugin
{
public:
	~ExamplePlugin() override = default;

	VT_DECLARE_PLUGIN_GUID("39091412-4860-4001-9CAD-A5CEE6DD8F6E"_guid);

	inline uint32_t GetVersion() const override { return 1; }
	inline StringView GetName() const override { return "ExamplePlugin"; }
	inline StringView GetDescription() const override { return "Test"; }
	inline StringView GetCategory() const override { return "None"; }

	void Initialize() override;
	void Shutdown() override;

private:
};

VT_REGISTER_PLUGIN(ExamplePlugin);
