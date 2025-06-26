using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltEntryPoint : CommonVoltLibProject
	{
        public VoltEntryPoint() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-EntryPoint";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

			conf.AddPrivateDependency<VoltPlatforms>(target);
			conf.AddPublicDependency<VoltApplication>(target);
		}
	}
}
