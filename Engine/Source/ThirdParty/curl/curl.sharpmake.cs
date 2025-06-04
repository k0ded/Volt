using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class curl : Sharpmake.Project
    {
        public curl() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "curl";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string targetOptimization = target.Optimization.ToString();

			conf.IncludePaths.Add(@"[project.RootPath]\include");
            conf.TargetFileName = @"libcurl";
			conf.TargetPath = @"[project.RootPath]\lib\";
			conf.TargetLibraryPath = @"[project.RootPath]\lib\";
			conf.Output = Configuration.OutputType.Dll;

			conf.AddPublicDependency<zlib>(target);

			string binPath = @"[project.RootPath]\lib\";
			conf.EventPostBuild.Add(@"copy /Y " + "\"" + binPath + "\\" + "libcurl" + ".dll\"" + " \"" + Globals.BinariesDirectory + "\"");

			conf.TargetCopyFiles.Add(Globals.BinariesDirectory + "\\libcrypto-1_1-x64.dll");
			conf.TargetCopyFiles.Add(Globals.BinariesDirectory + "\\libssl-1_1-x64.dll");
			conf.TargetCopyFiles.Add(Globals.BinariesDirectory + "\\libcrypto-3-x64.dll");
			conf.TargetCopyFiles.Add(Globals.BinariesDirectory + "\\libssl-3-x64.dll");
		}
	}
}
