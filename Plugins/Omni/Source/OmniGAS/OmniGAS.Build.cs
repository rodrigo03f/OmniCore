using UnrealBuildTool;

public class OmniGAS : ModuleRules
{
	public OmniGAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"OmniCore"
			}
		);
	}
}
