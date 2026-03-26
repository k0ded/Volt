using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltImGui : CommonVoltDllProject
    {
        public VoltImGui() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-ImGui";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

			conf.AddPrivateDependency<imgui>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);
			conf.AddPrivateDependency<WindowModule>(target);
			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<InputModule>(target);
			conf.AddPrivateDependency<RHIModule>(target);
			conf.AddPrivateDependency<LogModule>(target);

			conf.AddPrivateDependency<VoltRenderCore>(target);
			conf.AddPrivateDependency<VoltFileSystem>(target);

			conf.IncludePrivatePaths.Add(Path.Combine(Globals.ThirdPartyDirectory, "imgui-notify"));
		}
	}
}
