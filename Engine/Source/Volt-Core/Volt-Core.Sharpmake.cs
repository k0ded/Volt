using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltCore : CommonVoltDllProject
    {
        public VoltCore() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Core";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtcorepch.h";
            conf.PrecompSource = "vtcorepch.cpp";

			conf.AddPrivateDependency<VoltPlatforms>(target);
			conf.AddPrivateDependency<VoltFileSystem>(target);

			conf.AddPrivateDependency<LogModule>(target);
			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);
			//conf.AddPrivateDependency<AssetSystemModule>(target);
			//conf.AddPrivateDependency<EntitySystemModule>(target);
			conf.AddPrivateDependency<WindowModule>(target);
			conf.AddPrivateDependency<RHIModule>(target);
		}
    }
}
