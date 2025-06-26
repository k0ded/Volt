using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class ProjectUpgradeClient : CommonVoltExeProject
    {
        public ProjectUpgradeClient()
        {
            Name = "ProjectUpgradeClient";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.VcxprojUserFile = new Configuration.VcxprojUserFileSettings
			{
				LocalDebuggerWorkingDirectory = Globals.EngineDirectory
			};

			conf.SolutionFolder = "Programs";
        }

        public override void ConfigureWin64(Configuration conf, CommonTarget target)
        {
            base.ConfigureWin64(conf, target);

			conf.AddPrivateDependency<SubSystemModule>(target);
			conf.AddPrivateDependency<EventSystemModule>(target);
			conf.AddPrivateDependency<VoltEntryPoint>(target);
			conf.AddPrivateDependency<VoltRenderer>(target);
			conf.AddPrivateDependency<VoltPlatforms>(target);
			conf.AddPrivateDependency<VoltCore>(target);
			conf.AddPrivateDependency<imgui>(target);
			conf.AddPrivateDependency<yaml>(target);

			conf.AddPrivateDependency<VoltApplication>(target);

            conf.LibraryFiles.Add(
                "crypt32.lib",
                "Bcrypt.lib",

                "Winmm.lib",
                "Version.lib"
                );

			//we have to add the d3d12 to the exe folder
			string d3d12FolderPath = Path.Combine(Globals.ThirdPartyDirectory, "d3d12", "Binaries");
			conf.EventPostBuild.Add(@"copy /Y " + "\"" + d3d12FolderPath + "\\D3D12Core.dll\"" + " \"" + conf.TargetPath + "\"");
            conf.EventPostBuild.Add(@"copy /Y " + "\"" + d3d12FolderPath + "\\D3D12Core.pdb\"" + " \"" + conf.TargetPath + "\"");
            conf.EventPostBuild.Add(@"copy /Y " + "\"" + d3d12FolderPath + "\\d3d12SDKLayers.dll\"" + " \"" + conf.TargetPath + "\"");
            conf.EventPostBuild.Add(@"copy /Y " + "\"" + d3d12FolderPath + "\\d3d12SDKLayers.pdb\"" + " \"" + conf.TargetPath + "\"");
        }
    }
}
