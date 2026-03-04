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
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeProperty(PropertyChangedEvent);

		ReconstructNode();

		if (UEdGraph* Graph = GetGraph())
		{
			Graph->NotifyGraphChanged();
		}
	}
#endif

	virtual void AllocateDefaultPins() override
	{
		if (NodeType == EDialogueGraphNodeType::Start)
		{
			// Start: ONLY one output pin
			UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("Out_0")));
			if (OutPin)
			{
				OutPin->PinFriendlyName = FText::FromString(TEXT("Start"));
			}
			return;
		}
		// Input
		UEdGraphPin* InPin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("In")));
		if (InPin)
		{
			InPin->PinFriendlyName = FText::FromString(TEXT("In"));
		}

		// Outputs
		for (int32 i = 0; i < NodeData.Outputs.Num(); i++)
		{
			const FName PinName(*FString::Printf(TEXT("Out_%d"), i));
			UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, PinName);

			if (OutPin)
			{
				OutPin->PinFriendlyName = NodeData.Outputs[i].Text.IsEmpty()
					? FText::FromString(FString::Printf(TEXT("Choice %d"), i))
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
};