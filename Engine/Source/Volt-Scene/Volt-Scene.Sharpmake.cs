using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltScene : CommonVoltDllProject
    {
        public VoltScene() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Scene";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vspch.h";
            conf.PrecompSource = "vspch.cpp";

			conf.AddPublicDependency<LogModule>(target);

			conf.AddPublicDependency<VoltCore>(target);
			conf.AddPublicDependency<VoltRenderer>(target);
			conf.AddPublicDependency<VoltPhysics>(target);
			conf.AddPublicDependency<VoltCoreComponents>(target);

			conf.AddPublicDependency<yaml>(target);
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
