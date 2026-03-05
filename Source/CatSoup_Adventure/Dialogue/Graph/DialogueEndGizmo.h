// DialogueEndGizmo.h - Visual marker for dialogue end. Connect choice pins here to mark "end of dialogue".
#pragma once

#include "EdGraph/EdGraphNode.h"
#include "DialogueGraphPins.h"
#include "DialogueEndGizmo.generated.h"

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueEndGizmo : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** ID for this exit (e.g. "Leave", "Accept"). Used to map "exit via this" → "next time start at Entry Point X". */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName EndNodeId;

	/** Events to broadcast when the dialogue ends via this End node. */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	TArray<FName> EventNames;

	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return false; }
};
