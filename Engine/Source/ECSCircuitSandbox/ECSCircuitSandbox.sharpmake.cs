using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class ECSCircuitSandbox : CommonVoltExeProject
    {
        public ECSCircuitSandbox()
        {
            Name = "ECSCircuitSandbox";
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "";

            conf.PrecompHeader = "ecscsbpch.h";
            conf.PrecompSource = "ecscsbpch.cpp";

            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
            {
                LocalDebuggerWorkingDirectory = Globals.EngineDirectory,
                LocalDebuggerCommandArguments = Globals.VtProjectFilePath
            };

			conf.AddPrivateDependency<VoltEntryPoint>(target);
			conf.AddPrivateDependency<VoltApplication>(target);
            conf.AddPrivateDependency<PlatformsModule>(target);

			conf.AddPrivateDependency<ECSCircuit>(target);

			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<InputModule>(target);
			conf.AddPrivateDependency<WindowModule>(target);
			conf.AddPrivateDependency<RHIModule>(target);
			conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);

            conf.AddPrivateDependency<glm>(target);

			Type gameProjectType = Type.GetType("VoltSharpmake.Game");
			if (gameProjectType != null)
			{
				conf.AddPrivateDependency(target, gameProjectType);
			}

			conf.AddPrivateDependency<CrashReportClient>(target, DependencySetting.OnlyBuildOrder);

            conf.AdditionalDebuggerCommands = Path.Combine(Globals.VtProjectDirectory, @"Project.vtproj");


			conf.IncludePaths.Add("Public/ECSCircuitSandbox");
			conf.IncludePaths.Add("Private/ECSCircuitSandbox");
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
