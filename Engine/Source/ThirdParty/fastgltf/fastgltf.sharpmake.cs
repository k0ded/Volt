using System;
using System.IO;

using Sharpmake;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class fastgltf : CommonThirdPartyLibProject
    {
        public fastgltf() : base()
        {
            Name = "fastgltf";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.IncludePaths.Add("include");

			conf.AddPrivateDependency<simdjson>(target);
		}
    }
}
