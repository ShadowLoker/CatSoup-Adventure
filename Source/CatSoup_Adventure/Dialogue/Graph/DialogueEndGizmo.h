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
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return true; }
	virtual bool CanDuplicateNode() const override { return false; }
};
