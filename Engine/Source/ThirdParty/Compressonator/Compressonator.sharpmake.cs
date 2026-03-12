using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class Compressonator : Sharpmake.Project
    {
        public Compressonator() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Compressonator";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string libSubFolder = "release";

			if (target.Optimization == Optimization.Debug)
			{
				libSubFolder = "debug";
			}

			string libDir = @"[project.RootPath]\lib\" + libSubFolder;

			conf.IncludePaths.Add(@"[project.RootPath]\include");
			conf.LibraryPaths.Add(libDir);
			conf.LibraryFiles.Add(
                "Compressonator_MD.lib",
                "CMP_Framework_MD.lib"
            );

			conf.EventPostBuild.Add("copy /Y" + "\"" + libDir + "\\EncodeWith_GPU.dll\"" + "\"" + Globals.EngineDirectory + "\"");
		}
	}
}
