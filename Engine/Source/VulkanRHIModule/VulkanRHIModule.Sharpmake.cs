using Sharpmake;
using System;
using System.IO;

namespace VoltSharpmake
{
	[Sharpmake.Generate]
	public class VulkanRHIModule : CommonVoltDllProject
	{
		public VulkanRHIModule()
		{
			AddTargets(CommonTarget.GetDefaultTargets());
			Name = "VulkanRHIModule";
		}

		public override void ConfigureAll(Configuration conf, CommonTarget target)
		{
			base.ConfigureAll(conf, target);

			conf.SolutionFolder = "Engine/RHI";

			conf.PrecompHeader = "vkpch.h";
			conf.PrecompSource = "vkpch.cpp";

			conf.AddPublicDependency<RHIModule>(target);
			conf.AddPublicDependency<GLFW>(target);
			conf.AddPublicDependency<LogModule>(target);
			conf.AddPublicDependency<imgui>(target);

			conf.AddPrivateDependency<VulkanMemoryAllocator>(target);
			conf.AddPrivateDependency<Aftermath>(target);
			conf.AddPrivateDependency<DXC>(target);
			conf.AddPrivateDependency<spirv_reflect>(target);
			conf.AddPrivateDependency<SPIRV_Tools>(target);

			string vulkanSDKPath = Path.Combine(Environment.GetEnvironmentVariable("VULKAN_SDK"), "Include");
			if (vulkanSDKPath != null)
			{
				conf.IncludePrivatePaths.Add(vulkanSDKPath);
			}

			string vulkanSDKLibPath = Path.Combine(Environment.GetEnvironmentVariable("VULKAN_SDK"), "Lib");
			if (vulkanSDKLibPath != null)
			{
				conf.LibraryPaths.Add(vulkanSDKLibPath);
				conf.LibraryFiles.Add("vulkan-1.lib");
			}

			conf.Options.Add(Options.Vc.Linker.IgnoreImportLibrary.Enable);
		}

		public override void ConfigureDebug(Configuration conf, CommonTarget target)
		{
			base.ConfigureDebug(conf, target);

			string vulkanSDKPath = Path.Combine(Environment.GetEnvironmentVariable("VULKAN_SDK"), "Lib");
			if (vulkanSDKPath != null)
			{
				conf.LibraryPaths.Add(vulkanSDKPath);
			}
		}

		public override void ConfigureDevelopment(Configuration conf, CommonTarget target)
		{
			base.ConfigureDevelopment(conf, target);

			string vulkanSDKPath = Path.Combine(Environment.GetEnvironmentVariable("VULKAN_SDK"), "Lib");
			if (vulkanSDKPath != null)
			{
				conf.LibraryPaths.Add(vulkanSDKPath);
			}
		}

		public override void ConfigureDist(Configuration conf, CommonTarget target)
		{
			base.ConfigureDist(conf, target);

			string vulkanSDKPath = Path.Combine(Environment.GetEnvironmentVariable("VULKAN_SDK"), "Lib");
			if (vulkanSDKPath != null)
			{
				conf.LibraryPaths.Add(vulkanSDKPath);
			}
		}

		public override void ConfigureClangCl(Configuration conf, CommonTarget target)
		{
			base.ConfigureClangCl(conf, target);

			conf.AdditionalCompilerOptions.Add(
				"-Wno-switch",
				"-Wno-delete-non-abstract-non-virtual-dtor"
			);
		}
	}
}
