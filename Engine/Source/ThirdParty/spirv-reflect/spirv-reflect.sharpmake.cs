using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class spirv_reflect : CommonThirdPartyLibProject
    {
        public spirv_reflect()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "spirv-reflect";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
            conf.IncludePaths.Add(@"spirv-reflect");

			conf.Options.Add(new Sharpmake.Options.Vc.Linker.DisableSpecificWarnings("4042"));
		}
	}
}
