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

			conf.AddPublicDependency<VoltCore>(target);
			conf.AddPublicDependency<VoltImGui>(target);

			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<InputModule>(target);

			conf.AddPublicDependency<VulkanRHIModule>(target);
			conf.AddPublicDependency<D3D12RHIModule>(target);

			conf.AddPublicDependency<VoltRenderer>(target);

			conf.AddPublicDependency<NavigationModule>(target);

			conf.AddPublicDependency<imgui>(target);

			//private
			conf.AddPrivateDependency<VoltPlatforms>(target);
		}
    }
}
