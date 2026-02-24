// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

	
public class CatSoup_Adventure : ModuleRules
{
	public CatSoup_Adventure(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateIncludePaths.Add(ModuleDirectory);
	}
}
