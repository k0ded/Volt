using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class CircuitSandbox : CommonVoltExeProject
    {
        public CircuitSandbox()
        {
            Name = "CircuitSandbox";
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "";

            conf.PrecompHeader = "csbpch.h";
            conf.PrecompSource = "csbpch.cpp";

            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
            {
                LocalDebuggerWorkingDirectory = Globals.EngineDirectory,
                LocalDebuggerCommandArguments = Globals.VtProjectFilePath
            };

			conf.AddPrivateDependency<VoltEntryPoint>(target);
			conf.AddPrivateDependency<VoltApplication>(target);
            conf.AddPrivateDependency<PlatformsModule>(target);

			conf.AddPrivateDependency<Circuit>(target);

			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<InputModule>(target);
			conf.AddPrivateDependency<WindowModule>(target);
			conf.AddPrivateDependency<RHIModule>(target);
			conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);

            conf.AddPrivateDependency<glm>(target);

			conf.AddPrivateDependency<VoltScene>(target); // temp
			conf.AddPrivateDependency<VoltRenderer>(target); // temp
			conf.AddPrivateDependency<AssetSystemModule>(target); // temp
			conf.AddPrivateDependency<VoltAssets>(target); // temp
			conf.AddPrivateDependency<EntitySystemModule>(target); // temp


			Type gameProjectType = Type.GetType("VoltSharpmake.Game");
			if (gameProjectType != null)
			{
				conf.AddPrivateDependency(target, gameProjectType);
			}

            conf.AdditionalDebuggerCommands = Path.Combine(Globals.VtProjectDirectory, @"Project.vtproj");


			conf.IncludePaths.Add("Public/CircuitSandbox");
			conf.IncludePaths.Add("Private/CircuitSandbox");
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
        {
            base.ConfigureWin64(conf, target);

            conf.LibraryFiles.Add(
                "crypt32.lib",
                "Bcrypt.lib",

                "Winmm.lib",
                "Version.lib",
				"ws2_32.lib"
				);
		}

		public override void ConfigureMSVC(Configuration conf, CommonTarget target)
        {
            base.ConfigureMSVC(conf, target);

            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4098","4217"));
        }
    }
}
