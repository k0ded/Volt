#pragma once

#include <Volt-Application/Application.h>

#include <gtest/gtest.h>

static const Volt::ApplicationCreationInfo s_applicationInfo
{
	.createMainWindow = false
};

extern std::filesystem::path g_workingDirectoryFilepath;

class ApplicationFixture : public Volt::Application, public testing::Test
{
public:
	ApplicationFixture()
		: Volt::Application(Volt::CommandLineBuilder({ { "workingdir", g_workingDirectoryFilepath.string() } }), s_applicationInfo)
	{ }

	void SetUp() override
	{
	}
};
