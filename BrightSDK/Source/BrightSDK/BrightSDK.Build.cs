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
				"Projects",
			}
		);

		string thirdPartyDir = Path.Combine(PluginDirectory, "ThirdParty");

		if (Target.Platform == UnrealTargetPlatform.IOS || Target.Platform == UnrealTargetPlatform.TVOS)
		{
			PublicAdditionalFrameworks.Add(
				new Framework(
					"brdsdk",
					Path.Combine(thirdPartyDir, "iOS", "brdsdk.xcframework"),
					"",
					true
				)
			);
		}
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string win64Dir = Path.Combine(thirdPartyDir, "Win64");

			PublicDelayLoadDLLs.Add("lum_sdk64.dll");
			RuntimeDependencies.Add(Path.Combine(win64Dir, "lum_sdk64.dll"));
			RuntimeDependencies.Add(Path.Combine(win64Dir, "net_updater64.exe"));
		}

	}
}
