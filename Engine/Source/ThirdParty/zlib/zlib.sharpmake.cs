using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class zlib : Sharpmake.Project
    {
        public zlib() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "zlib";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			conf.IncludePaths.Add(@"[project.RootPath]\include");
            conf.TargetFileName = @"zlib1";
			conf.TargetPath = @"[project.RootPath]\lib\";
			conf.TargetLibraryPath = @"[project.RootPath]\lib\";
			conf.Output = Configuration.OutputType.Dll;

			string binPath = @"[project.RootPath]\lib\";
			conf.EventPostBuild.Add(@"copy /Y " + "\"" + binPath + "\\" + "zlib" + ".dll\"" + " \"" + Globals.BinariesDirectory + "\"");
		}
	}
}
