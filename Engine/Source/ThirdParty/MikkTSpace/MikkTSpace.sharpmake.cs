using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class MikkTSpace : CommonThirdPartyLibProject
    {
        public MikkTSpace() : base()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "MikkTSpace";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.IncludePaths.Add("./");
			conf.IncludePrivatePaths.Add("MikkTSpace");
		}
    }
}
