using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltRenderer : CommonVoltDllProject
    {
        public VoltRenderer() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Renderer";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vrpch.h";
            conf.PrecompSource = "vrpch.cpp";

			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<VoltRenderCore>(target);
			conf.AddPublicDependency<EntitySystemModule>(target);
			conf.AddPublicDependency<AssetSystemModule>(target);
			conf.AddPublicDependency<WindowModule>(target);
			conf.AddPublicDependency<MosaicModule>(target);
			conf.AddPublicDependency<RHIModule>(target);
			conf.AddPublicDependency<VoltAnimation>(target);

			conf.AddPrivateDependency<libacc>(target);
			conf.AddPrivateDependency<meshoptimizer>(target);
			conf.AddPrivateDependency<METIS>(target);
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
