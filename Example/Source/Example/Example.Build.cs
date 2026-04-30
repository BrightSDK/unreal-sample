// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Example : ModuleRules
{
	public Example(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		string brightSdkDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../ThirdParty/BrightSdk"));
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG", "BrightSDK" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicAdditionalFrameworks.Add(
				new Framework(
					"brdsdk",
					"../../ThirdParty/BrightSdk/brdsdk.xcframework", 
					"",
					true
				)
			);
		}

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string win64Dll = Path.Combine(brightSdkDir, "lum_sdk64.dll");

			if (File.Exists(win64Dll))
			{
				RuntimeDependencies.Add("$(TargetOutputDir)/lum_sdk64.dll", win64Dll);
			}
		}
	}
}
