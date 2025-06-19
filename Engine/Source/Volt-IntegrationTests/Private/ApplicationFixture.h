#pragma once

#include <Volt/Core/Application.h>

#include <gtest/gtest.h>

static const Volt::ApplicationInfo s_applicationInfo
{
	.createMainWindow = false
};

extern std::filesystem::path g_workingDirectoryFilepath;

class ApplicationFixture : public Volt::Application, public testing::Test
{
public:
	ApplicationFixture()
		: Volt::Application(s_applicationInfo, Volt::CommandLineBuilder({ { "workingdir", g_workingDirectoryFilepath.string() } }))
	{ }

	void SetUp() override
	{
	}
};
