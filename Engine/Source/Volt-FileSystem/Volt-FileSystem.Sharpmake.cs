using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltFileSystem : CommonVoltDllProject
	{
        public VoltFileSystem() : base()
        {
            Name = "Volt-FileSystem";
		}

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
			base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

			conf.AddPrivateDependency<VoltPlatforms>(target);
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);
		}
	}
}
