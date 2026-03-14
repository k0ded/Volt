using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class AssetSystemModule : CommonVoltDllProject
    {
        public AssetSystemModule()
        {
            Name = "AssetSystemModule";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

            conf.PrecompHeader = "aspch.h";
            conf.PrecompSource = "aspch.cpp";

            conf.AddPrivateDependency<LogModule>(target);
            conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<EventSystemModule>(target);

			conf.AddPrivateDependency<VoltCore>(target);
			conf.AddPrivateDependency<VoltPlatforms>(target);
			conf.AddPrivateDependency<VoltFileSystem>(target);
		}
    }
}
