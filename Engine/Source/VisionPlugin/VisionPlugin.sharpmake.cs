using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VisionPlugin : CommonVoltPluginProject
    {
        public VisionPlugin()
        {
            Name = "VisionPlugin";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Plugins";

            conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<EventSystemModule>(target);
			conf.AddPublicDependency<VoltScene>(target);
			conf.AddPublicDependency<EntitySystemModule>(target);
		}
	}
}
