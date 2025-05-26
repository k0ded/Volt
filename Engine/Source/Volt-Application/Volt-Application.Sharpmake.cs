using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltApplication : CommonVoltDllProject
    {
        public VoltApplication() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Application";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vtapppch.h";
            conf.PrecompSource = "vtapppch.cpp";

			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<VoltCore>(target);

			conf.AddPublicDependency<VulkanRHIModule>(target);
			conf.AddPublicDependency<D3D12RHIModule>(target);

			conf.AddPublicDependency<VoltRenderer>(target);
		}
    }
}
