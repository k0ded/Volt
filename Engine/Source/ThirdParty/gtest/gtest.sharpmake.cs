using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Export]
    public class gtest : Sharpmake.Project
    {
        public gtest() : base(typeof(CommonTarget))
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "gtest";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
            string subDir = "Release";

            if (target.Optimization == Optimization.Debug)
            {
                subDir = "Debug";
            }

            conf.IncludePaths.Add(@"[project.RootPath]\include");
            conf.LibraryPaths.Add(@"[project.RootPath]\lib\" + subDir);
            conf.Output = Configuration.OutputType.Lib;
        }
    }
}
