#pragma once

#include "AssetTypeActions_Base.h"  // Use this, not ClassTypeActions_Base.h
#include "DialogueAssetEditorToolkit.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "Templates/SharedPointer.h" // if not already included

class FDialogueAssetTypeActions : public FAssetTypeActions_Base
{
public:
	FDialogueAssetTypeActions() {}  // Default constructor, no parameters

	virtual FText GetName() const override
	{
		return NSLOCTEXT("AssetTypeActions", "DialogueAsset", "Dialogue Asset");
	}

	virtual FColor GetTypeColor() const override
	{
		return FColor(0, 128, 255);
	}

	virtual UClass* GetSupportedClass() const override
	{
		return UDialogueAsset::StaticClass();
	}

	virtual uint32 GetCategories() override
	{
		return EAssetTypeCategories::Gameplay;
	}

	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = nullptr) override
	{
		for (UObject* Obj : InObjects)
		{
			if (UDialogueAsset* Asset = Cast<UDialogueAsset>(Obj))
			{
				// Create the toolkit as a shared reference so SharedThis/CreateSP works correctly
				TSharedRef<FDialogueAssetEditorToolkit> NewEditor = MakeShared<FDialogueAssetEditorToolkit>();
				NewEditor->Initialize(Asset);
			}
		}
	}
};