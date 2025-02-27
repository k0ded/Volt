using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class MosaicModule : CommonVoltDllProject
    {
        public MosaicModule()
        {
            Name = "MosaicModule";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

            conf.PrecompHeader = "mcpch.h";
            conf.PrecompSource = "mcpch.cpp";

            conf.AddPublicDependency<LogModule>(target);
        }
    }
}
