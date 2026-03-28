using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltAnimation : CommonVoltDllProject
    {
        public VoltAnimation() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Animation";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vapch.h";
            conf.PrecompSource = "vapch.cpp";

			conf.AddPrivateDependency<LogModule>(target);
			conf.AddPrivateDependency<AssetSystemModule>(target);
			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<EntitySystemModule>(target);

			conf.AddPrivateDependency<VoltPlatforms>(target);
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
