// DialogueGraphSchema.h
#pragma once

#include "Dialogue/Graph/DialogueGraphNode.h"
#include "EdGraph/EdGraphSchema.h"
#include "DialogueGraphSchema.generated.h"

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override
	{
		if (!A || !B) return false;

		const FPinConnectionResponse Resp = CanCreateConnection(A, B);
		if (Resp.Response == CONNECT_RESPONSE_DISALLOW) return false;

		// Ensure A is output, B is input
		if (A->Direction == EGPD_Input)
		{
			Swap(A, B);
		}

		// Optional: enforce single outgoing link from Start
		if (A->GetOwningNode() && Cast<UDialogueGraphNode>(A->GetOwningNode()) &&
			Cast<UDialogueGraphNode>(A->GetOwningNode())->NodeType == EDialogueGraphNodeType::Start)
		{
			A->BreakAllPinLinks(true);
		}

		return Super::TryCreateConnection(A, B);
	}
};