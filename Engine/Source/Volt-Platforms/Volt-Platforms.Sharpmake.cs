using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltPlatforms : CommonVoltDllProject
	{
        public VoltPlatforms() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Platforms";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);

			conf.LibraryFiles.Add("Dbghelp.lib");
		}
	}
}
