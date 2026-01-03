using Sharpmake;
using System.Reflection;
using System;
using System.Linq;
using System.IO;
using System.Collections.Generic;

namespace VoltSharpmake
{ 
    [Sharpmake.Generate]
    public class VoltSolution : CommonSolution
    {
        public VoltSolution()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt";
        }

		private void AddConfigFiles(Configuration conf)
		{
			List<string> configFilesToAdd = new List<string>();

			// Check engine config directory
			{
				string configDir = Path.Combine(Globals.EngineDirectory, "Config");
				if (Directory.Exists(configDir))
				{
					string[] filesInDirectory = Directory.GetFiles(configDir);

					foreach (string filePath in filesInDirectory)
					{
						if (Path.GetExtension(filePath) == ".ini")
						{
							configFilesToAdd.Add(filePath);
						}
					}
				}
			}

			// Check game config directory
			{
				string configDir = Path.Combine(Globals.VtProjectDirectory, "Config");
				if (Directory.Exists(configDir))
				{
					string[] filesInDirectory = Directory.GetFiles(configDir);

					foreach (string filePath in filesInDirectory)
					{
						if (Path.GetExtension(filePath) == ".ini")
						{
							configFilesToAdd.Add(filePath);
						}
					}
				}
			}

			conf.Solution.ExtraItems["Configs"] = new Strings(configFilesToAdd);
		}

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

			conf.SolutionFileName = "[solution.Name]_[target.DevEnv]";

            //Sharpmake project, special case since it isnt a CommonProject
            conf.AddProject<SharpmakeProject>(target, true);

            foreach (Type projectType in Assembly.GetExecutingAssembly().GetTypes().Where(t => !t.IsAbstract && t.IsSubclassOf(typeof(CommonProject))))
			{
				conf.AddProject(projectType, target);
			}

			conf.Solution.ExtraItems["Solution Items"] = new Strings(Path.Combine(Globals.RootDirectory, ".editorconfig"));
			AddConfigFiles(conf);

			conf.SetStartupProject<Sandbox>();
		}
	}
}
