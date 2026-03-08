// DialogueStartGizmo.h - Singleton visual marker for where the dialogue starts.
// Not a dialogue node type; a gizmo you connect to a node's "From" pin.
#pragma once

#include "EdGraph/EdGraphNode.h"
#include "DialogueGraphPins.h"
#include "DialogueStartGizmo.generated.h"

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueStartGizmo : public UEdGraphNode
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual bool CanDuplicateNode() const override { return false; }
#endif

};
