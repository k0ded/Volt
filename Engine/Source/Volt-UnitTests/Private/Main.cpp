#include <gtest/gtest.h>

#include <CoreUtilities/Filesystem/Path.h>

Filesystem::Path g_workingDirectoryFilepath;

int main(int argc, char** argv)
{
	if (argc > 0)
	{
		g_workingDirectoryFilepath = argv[0];

		if (g_workingDirectoryFilepath.ParentPath().Stem() == "Binaries")
		{
			g_workingDirectoryFilepath = g_workingDirectoryFilepath.ParentPath();
		}
		else
		{
			while (g_workingDirectoryFilepath.Stem() != "Engine" && g_workingDirectoryFilepath.HasParentPath())
			{
				g_workingDirectoryFilepath = g_workingDirectoryFilepath.ParentPath();
			}
		}
	}

	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
