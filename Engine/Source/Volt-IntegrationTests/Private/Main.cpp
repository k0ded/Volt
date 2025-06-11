#include <gtest/gtest.h>

#include <filesystem>

std::filesystem::path g_workingDirectoryFilepath;

int main(int argc, char** argv)
{
	if (argc > 0)
	{
		g_workingDirectoryFilepath = argv[0];

		if (g_workingDirectoryFilepath.parent_path().stem() == "Binaries")
		{
			g_workingDirectoryFilepath = g_workingDirectoryFilepath.parent_path();
		}
		else
		{
			while (g_workingDirectoryFilepath.stem() != "Engine" && g_workingDirectoryFilepath.has_parent_path())
			{
				g_workingDirectoryFilepath = g_workingDirectoryFilepath.parent_path();
			}
		}
	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
