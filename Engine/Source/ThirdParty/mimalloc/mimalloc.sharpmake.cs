using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class mimalloc : Sharpmake.Project
    {
        public mimalloc() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "mimalloc";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string libSubFolder = "Release";

			if (target.Optimization == Optimization.Debug)
			{
				libSubFolder = "Debug";
			}

			conf.IncludePaths.Add(@"[project.RootPath]\include");
            conf.TargetFileName = @"mimalloc";
			conf.TargetPath = @"[project.RootPath]\bin\" + libSubFolder;
			conf.TargetLibraryPath = @"[project.RootPath]\bin\" + libSubFolder;
			conf.Output = Configuration.OutputType.Dll;
		}
	}
}
