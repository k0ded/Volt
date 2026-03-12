using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class VoltAudio : CommonVoltLibProject
    {
        public VoltAudio()
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "Volt-Audio";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine";

            conf.PrecompHeader = "vaudiopch.h";
            conf.PrecompSource = "vaudiopch.cpp";

            conf.AddPublicDependency<LogModule>(target);

			conf.AddPrivateDependency<EntitySystemModule>(target);

            conf.AddPublicDependency<wwise>(target);
            conf.AddPublicDependency<fmod>(target);
        }
    }
}
