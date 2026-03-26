#pragma once

#include <Volt-Core/Plugin/Plugin.h>

#include <EntitySystem/ComponentReflection.h>
#include <EntitySystem/ComponentRegistry.h>

class PLUGIN_API VisionPlugin : public Volt::Plugin
{
public:
	~VisionPlugin() override = default;

	VT_DECLARE_PLUGIN_GUID("1033DFF7-F5F3-4127-A404-5ECBAB06E2CE"_guid);

	inline uint32_t GetVersion() const override { return 1; }
	inline StringView GetName() const override { return "VisionPlugin"; }
	inline StringView GetDescription() const override { return "Camera Plugin"; }
	inline StringView GetCategory() const override { return "None"; }

	void Initialize() override;
	void Shutdown() override;

private:
};

struct VisionTest
{
	float yoooyo;

	static void ReflectType(Volt::TypeDesc<VisionTest>& reflect)
	{
		reflect.SetGUID("{688379F2-1D63-473C-9A13-A72065822774}"_guid);
		reflect.SetLabel("TestThing");

		reflect.AddMember(&VisionTest::yoooyo, 'yo', "TEST YO", "", 500.f);
	}
};

VT_REGISTER_PLUGIN(VisionPlugin);
