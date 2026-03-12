using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltIntegrationTests : CommonVoltExeProject
    {
        public VoltIntegrationTests() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-IntegrationTests";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
			{
				LocalDebuggerWorkingDirectory = Globals.EngineDirectory,
				LocalDebuggerCommandArguments = Globals.VtProjectFilePath
			};

			conf.SolutionFolder = "Tests";
			conf.Options.Add(Sharpmake.Options.Vc.Linker.SubSystem.Console);

			conf.AddPublicDependency<gtest>(target);
			conf.AddPublicDependency<VoltRenderCore>(target);
			conf.AddPublicDependency<VoltApplication>(target);
			conf.AddPublicDependency<imgui>(target);
        }
    }
}
