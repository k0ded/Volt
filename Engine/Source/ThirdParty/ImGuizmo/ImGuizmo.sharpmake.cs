using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class ImGuizmo : CommonThirdPartyDllProject
    {
        public ImGuizmo()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "ImGuizmo";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.AddPrivateDependency<imgui>(target);
            conf.IncludePaths.Add(@"[project.RootPath]\[project.Name]" );

			conf.Defines.Add("USE_IMGUI_API");

		}
	}
}
