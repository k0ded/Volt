using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class CoreUtilities : CommonVoltDllProject
    {
        public CoreUtilities() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "CoreUtilities";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";

            conf.PrecompHeader = "cupch.h";
            conf.PrecompSource = "cupch.cpp";

            conf.AddPublicDependency<glm>(target);
            conf.AddPublicDependency<tracy>(target);
			conf.AddPublicDependency<nfd_extended>(target);
			conf.AddPublicDependency<yaml>(target);
			conf.AddPublicDependency<nlohmann>(target);
			conf.AddPublicDependency<zlib>(target);

			conf.IncludePaths.Add(Path.Combine(Globals.ThirdPartyDirectory, "unordered_dense\\include"));
        }
    }
}
