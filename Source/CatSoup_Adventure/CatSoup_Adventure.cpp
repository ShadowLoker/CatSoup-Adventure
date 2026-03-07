// Fill out your copyright notice in the Description page of Project Settings.

#include "CatSoup_Adventure.h"
#include "Modules/ModuleManager.h"
#include "Modules/ModuleManager.h"          // For FModuleManager
#include "AssetToolsModule.h"                // For FAssetToolsModule
#include "IAssetTools.h"                    // For IAssetTools interface
#include "Dialogue/Graph/DialogueAssetTypeActions.h" // For FDialogueAssetTypeActions
#include "Dialogue/Graph/DialogueGraphNodeFactory.h"
#include "EdGraphUtilities.h"

IMPLEMENT_MODULE(FCatSoup_Adventure, CatSoup_Adventure)

void FCatSoup_Adventure::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FDialogueAssetTypeActions()));

#if WITH_EDITOR
	DialogueNodeFactory = MakeShared<FDialogueGraphNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(DialogueNodeFactory);
#endif
}

void FCatSoup_Adventure::ShutdownModule()
{
#if WITH_EDITOR
	if (DialogueNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(DialogueNodeFactory);
		DialogueNodeFactory.Reset();
	}
#endif
}