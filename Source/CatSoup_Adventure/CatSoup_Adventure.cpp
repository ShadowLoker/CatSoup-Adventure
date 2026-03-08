// Fill out your copyright notice in the Description page of Project Settings.

#include "CatSoup_Adventure.h"

#if WITH_EDITOR
#include "AssetToolsModule.h"
#include "Dialogue/Graph/DialogueAssetTypeActions.h"
#include "Dialogue/Graph/DialogueGraphNodeFactory.h"
#include "EdGraphUtilities.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#endif

IMPLEMENT_PRIMARY_GAME_MODULE(FCatSoup_Adventure, CatSoup_Adventure, "CatSoup_Adventure");

void FCatSoup_Adventure::StartupModule()
{
#if WITH_EDITOR
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTools.RegisterAssetTypeActions(MakeShareable(new FDialogueAssetTypeActions()));

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