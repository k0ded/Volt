using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltAssets : CommonVoltDllProject
    {
        public VoltAssets() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Assets";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtassetspch.h";
            conf.PrecompSource = "vtassetspch.cpp";

			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<AssetSystemModule>(target);
			conf.AddPublicDependency<yaml>(target);

			conf.AddPublicDependency<VoltCore>(target);
			conf.AddPublicDependency<VoltRenderer>(target);
			conf.AddPublicDependency<VoltMaterialGraph>(target);
		}
    }
}
