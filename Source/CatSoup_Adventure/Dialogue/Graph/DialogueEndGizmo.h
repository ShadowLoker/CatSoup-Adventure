// DialogueEndGizmo.h - Visual marker for dialogue end. Connect choice pins here to mark "end of dialogue".
#pragma once

#include "EdGraph/EdGraphNode.h"
#include "DialogueGraphPins.h"
#include "DialogueEndGizmo.generated.h"

class UDialogueAction;

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueEndGizmo : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** ID for this exit (e.g. "Leave", "Accept"). Used to map "exit via this" -> "next time start at Entry Point X". */
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName EndNodeId;

	/** Action instances to run when the dialogue ends via this End node. */
	UPROPERTY(EditAnywhere, Instanced, Category = "Dialogue")
	TArray<TObjectPtr<UDialogueAction>> Actions;
#endif

#if WITH_EDITOR
	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return false; }
#endif

};
