using UnrealBuildTool;

public class OmniRuntime : ModuleRules
{
	public OmniRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"InputCore",
				"UMG",
				"OmniCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Slate",
				"SlateCore"
			}
		);
	}
}
