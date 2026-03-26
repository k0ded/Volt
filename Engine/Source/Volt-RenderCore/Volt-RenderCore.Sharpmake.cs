using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltRenderCore : CommonVoltDllProject
    {
        public VoltRenderCore() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-RenderCore";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "rcpch.h";
            conf.PrecompSource = "rcpch.cpp";

            conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<RHIModule>(target);
            conf.AddPublicDependency<JobSystemModule>(target);

			conf.AddPublicDependency<VoltCore>(target);
			conf.AddPublicDependency<VoltFileSystem>(target);
			conf.AddPublicDependency<AssetSystemModule>(target);
        }

        public override void ConfigureClangCl(Configuration conf, CommonTarget target)
        {
            base.ConfigureClangCl(conf, target);

            conf.AdditionalCompilerOptions.Add(
                "-Wno-switch"
            );
        }
    }
}
