using UnrealBuildTool;

public class OmniExperimental : ModuleRules
{
	public OmniExperimental(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"OmniCore",
				"OmniRuntime"
			}
		);
	}
}
