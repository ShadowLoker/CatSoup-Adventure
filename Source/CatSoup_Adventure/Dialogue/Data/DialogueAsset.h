// Authored dialogue. Create via Content Browser -> Data Asset -> DialogueAsset.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Dialogue/Data/DialogueDataTypes.h"
#include "DialogueAsset.generated.h"

UCLASS(BlueprintType)
class CATSOUP_ADVENTURE_API UDialogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Default start node (from Start gizmo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName StartNodeId;

	/** All nodes. Key = node id; use this in Outputs' "Next Node" to link nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FDialogueNode> Nodes;

	/** Alternate entry points. Key = EntryPointId (e.g. "Return", "Continue"); Value = node id to jump to. Use Start(Asset, EntryPointId). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FName> EntryPoints;

	/** When you exit via End node X, next time start at Entry Point Y. Key = EndNodeId; Value = EntryPointId. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FName> EndToNextEntry;

	void CompileFromGraph();
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool IsValid() const;

	UPROPERTY()
	TObjectPtr<class UDialogueGraph> EditorGraph;
};
