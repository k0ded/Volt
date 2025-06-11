using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltUnitTests : CommonVoltExeProject
    {
        public VoltUnitTests() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-UnitTests";
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
        }
    }
}
