using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class D3D12RHIModule_New : CommonVoltDllProject
    {
        public D3D12RHIModule_New() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "D3D12RHIModule_New";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.SolutionFolder = "Engine/RHI";

            conf.PrecompHeader = "dxpch.h";
            conf.PrecompSource = "dxpch.cpp";

            conf.AddPublicDependency<RHIModule>(target);
            conf.AddPublicDependency<GLFW>(target);
            conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<imgui>(target);

            conf.AddPrivateDependency<Aftermath>(target);
            conf.AddPrivateDependency<DXC>(target);

            conf.IncludePrivatePaths.Add(Path.Combine(Globals.ThirdPartyDirectory, "d3d12"));
			conf.IncludePrivatePaths.Add("Public/" + "D3D12RHIModule");
			conf.IncludePrivatePaths.Add("Private/" + "D3D12RHIModule");

			conf.LibraryFiles.Add("d3d12.lib");
            conf.LibraryFiles.Add("DXGI.lib");
            conf.LibraryFiles.Add("dxguid.lib");
		}

		public override void ConfigureWin64(Configuration conf, CommonTarget target)
		{
			base.ConfigureWin64(conf, target);

			string d3d12FolderPath = Path.Combine(Globals.ThirdPartyDirectory, "d3d12", "Binaries");
			conf.TargetCopyFiles.Add(d3d12FolderPath + "\\D3D12Core.dll");
			conf.TargetCopyFiles.Add(d3d12FolderPath + "\\D3D12Core.pdb");
			conf.TargetCopyFiles.Add(d3d12FolderPath + "\\d3d12SDKLayers.dll");
			conf.TargetCopyFiles.Add(d3d12FolderPath + "\\d3d12SDKLayers.pdb");
		}

		public override void ConfigureClangCl(Configuration conf, CommonTarget target)
        {
            base.ConfigureClangCl(conf, target);

            conf.AdditionalCompilerOptions.Add(
                "-Wno-switch",
                "-Wno-delete-non-abstract-non-virtual-dtor",
                "-Wno-tautological-undefined-compare",
                "-Wno-unused-const-variable",
                "-Wno-unused-value",
                "-Wno-unused-private-field"
            );
        }
    }
}
