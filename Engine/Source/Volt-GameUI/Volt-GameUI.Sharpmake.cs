using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltGameUI : CommonVoltDllProject
    {
        public VoltGameUI() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-GameUI";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtguipch.h";
            conf.PrecompSource = "vtguipch.cpp";

			conf.AddPublicDependency<EntitySystemModule>(target);
			conf.AddPublicDependency<VoltRenderer>(target);
		}
    }
}
