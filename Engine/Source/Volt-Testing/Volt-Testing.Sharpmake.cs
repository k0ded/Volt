using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltTesting : CommonVoltExeProject
    {
        public VoltTesting() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Testing";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
			{
				LocalDebuggerWorkingDirectory = Globals.EngineDirectory,
				LocalDebuggerCommandArguments = Globals.VtProjectFilePath
			};

			conf.SolutionFolder = "Testing";
			conf.Options.Add(Sharpmake.Options.Vc.Linker.SubSystem.Console);

			conf.AddPublicDependency<gtest>(target);
			conf.AddPublicDependency<VoltRenderCore>(target);
			conf.AddPublicDependency<Volt>(target);
			conf.AddPublicDependency<imgui>(target);
        }
    }
}
