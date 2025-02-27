using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltCoreComponents : CommonVoltDllProject
    {
        public VoltCoreComponents() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-CoreComponents";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtccpch.h";
            conf.PrecompSource = "vtccpch.cpp";

			conf.AddPublicDependency<LogModule>(target);

			conf.AddPublicDependency<VoltAssets>(target);
			conf.AddPublicDependency<VoltRenderer>(target);
		}
    }
}
