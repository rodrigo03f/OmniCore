using UnrealBuildTool;

public class OmniEditor : ModuleRules
{
	public OmniEditor(ReadOnlyTargetRules Target) : base(Target)
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
				"OmniGAS"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"UnrealEd",
				"Slate",
				"SlateCore"
			}
		);
	}
}
