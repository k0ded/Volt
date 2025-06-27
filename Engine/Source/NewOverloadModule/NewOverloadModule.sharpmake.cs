using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class NewOverloadModule : CommonVoltLibProject
    {
        public NewOverloadModule()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "NewOverloadModule";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";
			conf.AddPublicDependency<mimalloc>(target);
        }
    }
}
