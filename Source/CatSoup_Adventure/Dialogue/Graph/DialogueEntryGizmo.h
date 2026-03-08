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
#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Dialogue")
	FName EntryPointId;
#endif

#if WITH_EDITOR
	virtual void AllocateDefaultPins() override;
	virtual void ReconstructNode() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return true; }
#endif

};
