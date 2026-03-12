using System.IO;
using Sharpmake;

namespace VoltSharpmake
{
	[Sharpmake.Generate]
	public class VoltShaders : CommonVoltProject
	{
		public VoltShaders()
		{
			Name = "Volt-Shaders";

			SourceRootPath = Path.Combine(Globals.EngineDirectory, "Engine", "Shaders", "Source");
			SourceFilesExtensions.Add(".hlsl", ".hlsli");
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
		{
			base.ConfigureAll(conf, target);

			conf.IsExcludedFromBuild = true;
			conf.SolutionFolder = "Engine";
		}
	}
}
