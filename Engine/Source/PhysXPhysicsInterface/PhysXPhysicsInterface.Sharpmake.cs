using Sharpmake;

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
			conf.AddPrivateDependency<FileSystemModule>(target);

			conf.AddPublicDependency<LogModule>(target);
            conf.AddPublicDependency<PhysicsInterface>(target);

			conf.EventPostBuild.Add(@"copy /Y " + "\"" + conf.TargetPath + "\\" + Name + ".dll\"" + " \"" + Globals.BinariesDirectory + "\"");

			conf.Options.Add(Options.Vc.Linker.IgnoreImportLibrary.Enable);
		}
	}
}
