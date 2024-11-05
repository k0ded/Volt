using System;
using System.IO;

namespace VoltSharpmake
{
    [Sharpmake.Generate]
    public class PhysXPhysicsInterface : CommonVoltDllProject
    {
        public PhysXPhysicsInterface() 
        {
            AddTargets(CommonTarget.GetDefaultTargets());
            Name = "PhysXPhysicsInterface";
        }

        public override void ConfigureAll(Configuration conf, CommonTarget target)
        {
            base.ConfigureAll(conf, target);

            conf.SolutionFolder = "Engine/Physics";

            conf.PrecompHeader = "pxpch.h";
            conf.PrecompSource = "pxpch.cpp";

			conf.AddPrivateDependency<PhysX>(target);

            conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<PhysicsInterface>(target);
		}
    }
}
