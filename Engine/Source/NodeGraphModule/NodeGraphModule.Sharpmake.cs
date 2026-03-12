using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class NodeGraphModule : CommonVoltDllProject
    {
        public NodeGraphModule()
        {
            Name = "NodeGraphModule";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

            conf.PrecompHeader = "nodepch.h";
            conf.PrecompSource = "nodepch.cpp";

            conf.AddPrivateDependency<LogModule>(target);

			conf.AddPrivateDependency<VoltCore>(target);
        }
    }
}
