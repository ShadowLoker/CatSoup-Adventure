// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

	
public class CatSoup_Adventure : ModuleRules
{
	public CatSoup_Adventure(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EditorStyle",  
			"AssetTools", 
			"UnrealEd", 
			"PropertyEditor",
			"Slate", 
			"SlateCore", 
			"WorkspaceMenuStructure",
			"GraphEditor",
			"BlueprintGraph",   // Add this to resolve UEdGraphSchema_K2 symbols
			"KismetCompiler"    // Sometimes also needed for Blueprint compilation support
		});
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateIncludePaths.Add(ModuleDirectory);
	}
}
