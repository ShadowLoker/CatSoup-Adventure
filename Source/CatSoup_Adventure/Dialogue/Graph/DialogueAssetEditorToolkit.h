// DialogueAssetEditor.h
#pragma once
#include "Toolkits/AssetEditorToolkit.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "DialogueGraph.h"
#include "DialogueGraphNode.h"

class FDialogueAssetEditorToolkit : public FAssetEditorToolkit
{
public:
	void Initialize(UDialogueAsset* InDialogueAsset);
	static const FName DialogueEditorDetailsTabId;
	TSharedPtr<IDetailsView> DetailsView;

protected:
	TWeakObjectPtr<UDialogueAsset> DialogueAsset;

	TSharedPtr<FUICommandList> GraphCommandList;

	void DeleteSelectedNodes();
	bool CanDeleteSelectedNodes() const;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	virtual void OnGraphSelectionChanged(const TSet<UObject*>& NewSelection);
	
	TSharedRef<SDockTab> SpawnTab_GraphEditor(const FSpawnTabArgs& Args);
	TSharedPtr<SGraphEditor> GraphEditor;

	// Graph instance representing this asset's data
	UPROPERTY()
	TObjectPtr<UDialogueGraph> DialogueGraph;
};