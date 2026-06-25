using System;
using System.IO;

namespace VoltSharpmake
{
	[Sharpmake.Generate]
	public class ECSCircuit : CommonVoltDllProject
	{
		public ECSCircuit()
		{
			Name = "ECSCircuit";
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
		{
			base.ConfigureAll(conf, target);

			conf.SolutionFolder = "Engine";

			conf.PrecompHeader = "ecscircuitpch.h";
			conf.PrecompSource = "ecscircuitpch.cpp";


			conf.AddPublicDependency<VoltRenderCore>(target);
			conf.AddPrivateDependency<VoltApplication>(target);
			conf.AddPrivateDependency<VoltRenderer>(target);

			conf.AddPublicDependency<RHIModule>(target);
			conf.AddPublicDependency<WindowModule>(target);
			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<EventSystemModule>(target);
			conf.AddPublicDependency<InputModule>(target);
			conf.AddPublicDependency<VoltAssets>(target);

			conf.IncludePaths.Add(Path.Combine(Globals.ThirdPartyDirectory, "entt\\include"));
		}
	}
}
