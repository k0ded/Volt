using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class Sandbox_New : CommonVoltExeProject
    {
        public Sandbox_New()
        {
            Name = "Sandbox_New";
			SourceFiles.Add("Sandbox_New.rc");
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "";

            conf.PrecompHeader = "sbpch.h";
            conf.PrecompSource = "sbpch.cpp";

            conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
            {
                LocalDebuggerWorkingDirectory = Globals.EngineDirectory,
                LocalDebuggerCommandArguments = Globals.VtProjectFilePath
            };

            conf.AddPrivateDependency<VoltApplication>(target);
			conf.AddPrivateDependency<VoltAssets>(target);
			conf.AddPrivateDependency<VoltScene>(target);
			conf.AddPrivateDependency<VoltGameUI>(target);
			conf.AddPrivateDependency<VoltEntryPoint>(target);
			conf.AddPrivateDependency<VoltCoreComponents>(target);
			conf.AddPrivateDependency<VoltRenderer>(target);
			conf.AddPrivateDependency<VoltPhysics>(target);
			conf.AddPrivateDependency<VoltAnimation>(target);

			conf.AddPrivateDependency<NavigationModule>(target);
			conf.AddPrivateDependency<InputModule>(target);
			conf.AddPrivateDependency<WindowModule>(target);
			conf.AddPrivateDependency<RHIModule>(target);
			conf.AddPrivateDependency<JobSystemModule>(target);
			conf.AddPrivateDependency<SubSystemModule>(target);

			conf.AddPrivateDependency<ImGuizmo>(target);
            conf.AddPrivateDependency<imgui_node_editor>(target);
            conf.AddPrivateDependency<p4>(target);
            conf.AddPrivateDependency<OpenSSL>(target);

            conf.AddPrivateDependency<nfd_extended>(target);
            conf.AddPrivateDependency<yaml>(target);
            conf.AddPrivateDependency<MosaicModule>(target);

            conf.AddPrivateDependency<glm>(target);
            conf.AddPrivateDependency<esfw>(target);

			Type gameProjectType = Type.GetType("VoltSharpmake.Game");
			if (gameProjectType != null)
			{
				conf.AddPrivateDependency(target, gameProjectType);
			}

			conf.AddPrivateDependency<PhysXPhysicsInterface>(target);

			conf.IncludePaths.Add(
                Path.Combine(Globals.ThirdPartyDirectory ,@"nlohmann/include"),
                Path.Combine(Globals.ThirdPartyDirectory ,@"cpp-httplib/include")
                );

            conf.AdditionalDebuggerCommands = Path.Combine(Globals.VtProjectDirectory, @"Project.vtproj");

            conf.Defines.Add("CPPHTTPLIB_OPENSSL_SUPPORT");

			conf.IncludePaths.Add("Public/Sandbox");
			conf.IncludePaths.Add("Private/Sandbox");
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
        {
            base.ConfigureWin64(conf, target);

            conf.LibraryFiles.Add(
                "crypt32.lib",
                "Bcrypt.lib",

                "Winmm.lib",
                "Version.lib"
                );
		}

		public override void ConfigureMSVC(Configuration conf, CommonTarget target)
        {
            base.ConfigureMSVC(conf, target);
            conf.AdditionalLinkerOptions.Add(
                "/WHOLEARCHIVE:MosaicModule"
                );

            conf.Options.Add(new Sharpmake.Options.Vc.Compiler.DisableSpecificWarnings("4098","4217"));
        }
    }
}
