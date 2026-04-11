using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class NsightAftermath : Sharpmake.Project
    {
        public NsightAftermath() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Aftermath";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string libDir = @"[project.RootPath]\lib\x64";
			string libName = "GFSDK_Aftermath_Lib.x64.lib";

			conf.TargetFileName = @"GFSDK_Aftermath_Lib.x64";
			conf.TargetPath = @"[project.RootPath]\lib\x64\";
			conf.TargetLibraryPath = @"[project.RootPath]\lib\x64\";

			conf.IncludePaths.Add(@"[project.RootPath]\include");
			conf.LibraryPaths.Add(libDir);
			conf.LibraryFiles.Add(libName);
			conf.Output = Configuration.OutputType.Dll;

			conf.EventPostBuild.Add("copy /Y" + "\"" + libDir + "\\" + libName + "\"\"" + conf.TargetPath + "\"");
		}
	}
}
