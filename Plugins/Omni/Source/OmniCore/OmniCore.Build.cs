using UnrealBuildTool;

public class OmniCore : ModuleRules
{
	public OmniCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags"
			}
		);
	}
}
