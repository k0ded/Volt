using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltPhysics : CommonVoltDllProject
    {
        public VoltPhysics() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Physics";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vppch.h";
            conf.PrecompSource = "vppch.cpp";

			conf.AddPublicDependency<PhysicsInterface>(target);

			conf.AddPrivateDependency<LogModule>(target);
			conf.AddPrivateDependency<AssetSystemModule>(target);
			conf.AddPrivateDependency<EntitySystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);

			conf.AddPrivateDependency<VoltCore>(target);
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
