using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class cpptrace : Sharpmake.Project
    {
        public cpptrace() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "cpptrace";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string targetOptimization = target.Optimization.ToString();

			conf.IncludePaths.Add(@"[project.RootPath]\include");
            conf.TargetFileName = @"cpptrace";
			conf.TargetPath = Path.Combine(@"[project.RootPath]\lib\", targetOptimization);
			conf.TargetLibraryPath = Path.Combine(@"[project.RootPath]\lib\", targetOptimization);
			conf.Output = Configuration.OutputType.Lib;

			conf.Defines.Add("CPPTRACE_STATIC_DEFINE");
			conf.ExportDefines.Add("CPPTRACE_STATIC_DEFINE");
        }
    }
}
