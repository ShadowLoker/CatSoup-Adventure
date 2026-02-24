// Authored dialogue. Create via Content Browser -> Data Asset -> DialogueAsset.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Dialogue/Data/DialogueDataTypes.h"
#include "DialogueAsset.generated.h"

UCLASS(BlueprintType)
class CATSOUP_ADVENTURE_API UDialogueAsset : public UPrimaryDataAsset
{
	/** Map key of the node to start from (e.g. "Start"). */
	GENERATED_BODY()

	/** All nodes. Key = node id; use this in Outputs' "Next Node" to link nodes. */
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	/** Returns true if StartNodeId exists in Nodes. */
	FName StartNodeId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<FName, FDialogueNode> Nodes;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool IsValid() const;
};
