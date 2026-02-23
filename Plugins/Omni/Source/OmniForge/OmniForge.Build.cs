using UnrealBuildTool;

public class OmniForge : ModuleRules
{
	public OmniForge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"OmniCore",
				"OmniRuntime",
				"OmniEditor"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Json",
				"UnrealEd",
				"AssetTools",
				"Projects",
				"Slate",
				"SlateCore"
			}
		);
	}
}
