using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class RenderDocPlugin : CommonVoltPluginProject
    {
        public RenderDocPlugin()
        {
            Name = "RenderDocPlugin";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Plugins";

            conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<RHIModule>(target);
            conf.AddPublicDependency<JobSystemModule>(target);
            conf.AddPublicDependency<EventSystemModule>(target);
			conf.AddPublicDependency<WindowModule>(target);
			conf.AddPublicDependency<VoltCore>(target);

			conf.IncludePrivatePaths.Add("ThirdParty");

			string srcDir = Path.Combine(SourceRootPath, "ThirdParty");

			conf.EventPostBuild.Add(@"copy /Y " + "\"" + srcDir + "\\renderdoc.dll\"" + " \"" + Globals.BinariesDirectory + "\"");
		}
	}
}
