using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class PhysicsInterface : CommonVoltDllProject
    {
        public PhysicsInterface() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "PhysicsInterface";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Physics";

            conf.PrecompHeader = "pipch.h";
            conf.PrecompSource = "pipch.cpp";

            conf.AddPublicDependency<LogModule>(target);
        }
    }
}
