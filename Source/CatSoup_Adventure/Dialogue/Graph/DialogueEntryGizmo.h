// DialogueEntryGizmo.h - Alternate entry point with a specific ID (e.g. "Return", "Continue").
// Connect to a dialogue node. Use Start(Asset, EntryPointId) to begin from this node.
#pragma once

#include "EdGraph/EdGraphNode.h"
#include "DialogueGraphPins.h"
#include "DialogueEntryGizmo.generated.h"

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueEntryGizmo : public UEdGraphNode
{
	GENERATED_BODY()

public:
	/** ID used when calling Start(Asset, EntryPointId). E.g. "Return", "Continue". Cannot be "Default" (reserved). */
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName EntryPointId;

	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return true; }
};
