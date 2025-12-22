using Sharpmake;
using System;
using System.Collections.Generic;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class libpng : CommonThirdPartyLibProject
    {
        public libpng()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "libpng";
        }

        [Configure()]
        public void Configure(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.IncludePaths.Add("./");
			conf.IncludePrivatePaths.Add("libpng");

			conf.AddPrivateDependency<zlib>(target);

			var excludedFolders = new List<string>();
			excludedFolders.Add("contrib");
			excludedFolders.Add("scripts");
			conf.SourceFilesBuildExcludeRegex.Add(@"\.*\\(" + string.Join("|", excludedFolders.ToArray()) + @")\\");
		}
    }
}
