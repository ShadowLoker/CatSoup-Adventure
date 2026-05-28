using UnrealBuildTool;

public class DialogueSystemEditor : ModuleRules
{
	public DialogueSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DialogueSystem",
		});

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
			"DesktopPlatform",
		});

		PrivateIncludePaths.Add(ModuleDirectory);
	}
}
