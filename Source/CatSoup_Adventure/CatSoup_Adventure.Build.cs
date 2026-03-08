using UnrealBuildTool;

public class CatSoup_Adventure : ModuleRules
{
	public CatSoup_Adventure(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"AppFramework",
				"AssetTools",
				"BlueprintGraph",
				"GraphEditor",
				"KismetCompiler",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"WorkspaceMenuStructure",
			});
		}

		PrivateIncludePaths.Add(ModuleDirectory);
	}
}
