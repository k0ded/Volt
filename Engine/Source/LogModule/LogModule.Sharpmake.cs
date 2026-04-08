using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class LogModule : CommonVoltDllProject
    {
        public LogModule() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "LogModule";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Modules";
            conf.IncludePrivatePaths.Add(Path.Combine(Globals.ThirdPartyDirectory, "spdlog/include"));
            conf.Defines.Add("_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING");
        }

		public override void ConfigureClangCl(Configuration conf, CommonTarget target)
		{
			base.ConfigureClangCl(conf, target);

			// #Note: Added because fmt inside of spdlog triggers this warning.
			conf.AdditionalCompilerOptions.Add(
				"-Wno-deprecated-literal-operator"	
			);
		}
    }
}
