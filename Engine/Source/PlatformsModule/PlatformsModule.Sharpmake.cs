using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class PlatformsModule : CommonVoltDllProject
	{
        public PlatformsModule() : base()
        {
            Name = "PlatformsModule";
		}

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
			base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";
			conf.AddPublicDependency<cpptrace>(target);
			conf.AddPrivateDependency<curl>(target);
			conf.AddPrivateDependency<LogModule>(target);
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);

			conf.LibraryFiles.Add("Dbghelp.lib");
			conf.LibraryFiles.Add("Shlwapi.lib");
		}
	}
}
