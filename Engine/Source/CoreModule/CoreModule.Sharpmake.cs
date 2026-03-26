using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class CoreModule : CommonVoltDllProject
	{
        public CoreModule() : base()
        {
            Name = "CoreModule";
		}

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
			base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

			conf.AddPrivateDependency<VoltPlatforms>(target);
			conf.AddPrivateDependency<VoltFileSystem>(target);

			conf.AddPublicDependency<nlohmann>(target);
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);
		}
	}
}
