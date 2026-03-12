using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class nlohmann : Sharpmake.Project
    {
        public nlohmann() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "nlohmann";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
			string targetOptimization = target.Optimization.ToString();

			conf.IncludePaths.Add(@"[project.RootPath]\include");
			conf.Output = Configuration.OutputType.None;
		}
	}
}
