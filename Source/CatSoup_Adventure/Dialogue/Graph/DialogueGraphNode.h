// DialogueGraphNode.h
#pragma once
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Dialogue/Data/DialogueDataTypes.h"
#include "DialogueGraphPins.h"
#include "DialogueGraphNode.generated.h"

UENUM()
enum class EDialogueGraphNodeType : uint8
{
	Start,
	Dialogue
};

const FName DialogueFlowPinCategory(TEXT("DialogueFlow"));

UCLASS()
class CATSOUP_ADVENTURE_API UDialogueGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EDialogueGraphNodeType NodeType = EDialogueGraphNodeType::Dialogue;

	UPROPERTY(EditAnywhere, Category="Dialogue")
	FDialogueNode NodeData;

	UPROPERTY(EditAnywhere, Category="Dialogue")
	FName NodeId;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& E) override
	{
		Super::PostEditChangeProperty(E);

		const FName Prop = E.GetPropertyName();
		const FName Member = E.GetMemberPropertyName();

		// Rebuild pins when Outputs array is edited (add/remove/reorder) or its members change
		if (Prop == GET_MEMBER_NAME_CHECKED(FDialogueNode, Outputs) ||
			Member == GET_MEMBER_NAME_CHECKED(FDialogueNode, Outputs))
		{
			Modify();

			if (NodeType != EDialogueGraphNodeType::Start && NodeData.Outputs.Num() == 0)
			{
				NodeData.Outputs.AddDefaulted();
				NodeData.Outputs[0].Text = FText::FromString(TEXT("Next"));
				NodeData.Outputs[0].NextNodeId = NAME_None;
			}

			ReconstructNode();
			if (UEdGraph* G = GetGraph()) G->NotifyGraphChanged();
		}
	}

	virtual void PostEditUndo() override
	{
		Super::PostEditUndo();
		ReconstructNode();
		if (UEdGraph* G = GetGraph()) G->NotifyGraphChanged();
	}
#endif

	virtual void AllocateDefaultPins() override
	{
		if (NodeType == EDialogueGraphNodeType::Start)
		{
			UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("Out_0")));
			if (OutPin) OutPin->PinFriendlyName = FText::FromString(TEXT("Start"));
			return;
		}

		// Ensure data first
		if (NodeData.Outputs.Num() == 0)
		{
			NodeData.Outputs.AddDefaulted();
			NodeData.Outputs[0].Text = FText::FromString(TEXT("Next"));
			NodeData.Outputs[0].NextNodeId = NAME_None;
		}

		UEdGraphPin* InPin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("In")));
		if (InPin) InPin->PinFriendlyName = FText::FromString(TEXT("Input"));

		for (int32 i = 0; i < NodeData.Outputs.Num(); i++)
		{
			const FName PinName(*FString::Printf(TEXT("Out_%d"), i));
			UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, PinName);

			if (OutPin)
			{
				OutPin->PinFriendlyName = NodeData.Outputs[i].Text.IsEmpty()
					? FText::FromString(FString::Printf(TEXT("Option %d"), i + 1))
					: NodeData.Outputs[i].Text;
			}
		}
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		if (NodeType == EDialogueGraphNodeType::Start)
			return FText::FromString(TEXT("Start"));

		return NodeData.Text.IsEmpty() ? FText::FromName(NodeId) : NodeData.Text;
	}

	virtual bool CanUserDeleteNode() const override { return NodeType != EDialogueGraphNodeType::Start; }
	virtual bool CanDuplicateNode() const override { return NodeType != EDialogueGraphNodeType::Start; }
	
};