// Authored dialogue. Create via Content Browser -> Data Asset -> DialogueAsset.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueDataTypes.h"
#include "DialogueAsset.generated.h"

UCLASS(BlueprintType)
class DIALOGUESYSTEM_API UDialogueAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Default start node (from Start gizmo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName StartNodeId;

	/** All nodes. Key = node id; use this in Outputs' "Next Node" to link nodes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TMap<FName, FDialogueNode> Nodes;

	/** Alternate entry points. Key = EntryPointId (e.g. "Return", "Continue"); Value = node id to jump to. Use Start(Asset, EntryPointId). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TMap<FName, FName> EntryPoints;

	/** When you exit via End node X, next time start at Entry Point Y. Key = EndNodeId; Value = EntryPointId. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TMap<FName, FName> EndToNextEntry;

#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCompileDialogueAsset, UDialogueAsset*);
	static FOnCompileDialogueAsset OnCompileDialogueAsset;

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool IsValid() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UEdGraph> EditorGraph;
#endif
};
