// LICENSE_CODE ZON

using System.IO;
using UnrealBuildTool;

public class BrightSDK : ModuleRules
{
	public BrightSDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);

		string brightSdkDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../../ThirdParty/BrightSdk"));

		if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.TVOS)
		{
			PublicAdditionalFrameworks.Add(
				new Framework(
					"brdsdk",
					Path.Combine(brightSdkDir, "brdsdk.xcframework"),
					"",
					true
				)
			);
		}

	}
}
