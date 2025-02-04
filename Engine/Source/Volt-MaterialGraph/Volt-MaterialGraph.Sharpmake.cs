using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltMaterialGraph : CommonVoltDllProject
    {
        public VoltMaterialGraph() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-MaterialGraph";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtmgpch.h";
            conf.PrecompSource = "vtmgpch.cpp";

			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<MosaicModule>(target);
			conf.AddPublicDependency<AssetSystemModule>(target);
			conf.AddPublicDependency<RHIModule>(target);
			conf.AddPublicDependency<yaml>(target);
		}
    }
}
