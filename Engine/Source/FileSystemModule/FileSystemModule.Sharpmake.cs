using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class FileSystemModule : CommonVoltDllProject
	{
        public FileSystemModule() : base()
        {
            Name = "FileSystemModule";
		}

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
			base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

			conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);

			conf.AddPublicDependency<VoltPlatforms>(target);
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);
		}
	}
}
