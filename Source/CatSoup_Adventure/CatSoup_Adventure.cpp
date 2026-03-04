// Fill out your copyright notice in the Description page of Project Settings.

#include "CatSoup_Adventure.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleManager.h"          // For FModuleManager
#include "AssetToolsModule.h"                // For FAssetToolsModule
#include "IAssetTools.h"                    // For IAssetTools interface
#include "Dialogue/Graph/DialogueAssetTypeActions.h" // For FDialogueAssetTypeActions


IMPLEMENT_MODULE(FCatSoup_Adventure, CatSoup_Adventure)

void FCatSoup_Adventure::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FDialogueAssetTypeActions()));
}

void FCatSoup_Adventure::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module. For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}