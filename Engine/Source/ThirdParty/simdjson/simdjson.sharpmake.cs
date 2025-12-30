using System;
using System.IO;

using Sharpmake;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class simdjson : CommonThirdPartyLibProject
    {
        public simdjson() : base()
        {
            Name = "simdjson";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);
			conf.IncludePaths.Add("./");
		}
    }
}
