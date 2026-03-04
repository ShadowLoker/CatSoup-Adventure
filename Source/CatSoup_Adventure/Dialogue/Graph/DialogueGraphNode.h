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

	/** When set before ReconstructNode, output at this index was removed; restore skips it so remaining outputs get correct links. */
	int32 RemovedOutputIndexDuringReconstruct = -1;

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

	virtual void ReconstructNode() override
	{
		Modify();

		// Cache existing connections by output index (Out_0=0, Out_1=1, ...)
		TArray<TArray<UEdGraphPin*>> SavedOutputLinks;
		TArray<UEdGraphPin*> SavedInputLinks;

		for (UEdGraphPin* Pin : Pins)
		{
			if (!Pin) continue;

			if (Pin->Direction == EGPD_Output)
			{
				FString PinStr = Pin->PinName.ToString();
				if (PinStr.StartsWith(TEXT("Out_")))
				{
					int32 Idx = FCString::Atoi(*PinStr.Mid(4));
					while (SavedOutputLinks.Num() <= Idx)
						SavedOutputLinks.AddDefaulted();
					for (UEdGraphPin* Linked : Pin->LinkedTo)
						if (Linked) SavedOutputLinks[Idx].Add(Linked);
				}
			}
			else if (Pin->Direction == EGPD_Input)
			{
				for (UEdGraphPin* Linked : Pin->LinkedTo)
					if (Linked) SavedInputLinks.Add(Linked);
			}
		}

		BreakAllNodeLinks(/*bSendsNodeNotif=*/false);
		Pins.Reset();
		AllocateDefaultPins();

		// Restore input connections
		UEdGraphPin* NewInputPin = nullptr;
		for (UEdGraphPin* Pin : Pins)
		{
			if (Pin && Pin->Direction == EGPD_Input) { NewInputPin = Pin; break; }
		}
		if (NewInputPin)
		{
			for (UEdGraphPin* Linked : SavedInputLinks)
				if (Linked) NewInputPin->MakeLinkTo(Linked);
		}

		// Restore output connections by index; new pin i gets links from old pin at (i + skip)
		// When RemovedOutputIndexDuringReconstruct = R, we skip old index R.
		const int32 Removed = RemovedOutputIndexDuringReconstruct;
		RemovedOutputIndexDuringReconstruct = -1;

		int32 NewIdx = 0;
		for (UEdGraphPin* Pin : Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output) continue;
			int32 OldIdx = NewIdx + (Removed >= 0 && NewIdx >= Removed ? 1 : 0);
			if (SavedOutputLinks.IsValidIndex(OldIdx))
			{
				for (UEdGraphPin* Linked : SavedOutputLinks[OldIdx])
					if (Linked) Pin->MakeLinkTo(Linked);
			}
			NewIdx++;
		}

		// Sync NodeData.Outputs[].NextNodeId from restored pin links (BreakPinLinks clears them)
		if (NodeType == EDialogueGraphNodeType::Dialogue)
		{
			for (UEdGraphPin* Pin : Pins)
			{
				if (!Pin || Pin->Direction != EGPD_Output) continue;
				int32 OutIdx = INDEX_NONE;
				FString S = Pin->PinName.ToString();
				if (S.StartsWith(TEXT("Out_"))) OutIdx = FCString::Atoi(*S.Mid(4));
				if (OutIdx >= 0 && NodeData.Outputs.IsValidIndex(OutIdx))
				{
					FName NewNext = NAME_None;
					if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
					{
						if (UDialogueGraphNode* ToNode = Cast<UDialogueGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
							if (ToNode->NodeType != EDialogueGraphNodeType::Start)
								NewNext = ToNode->NodeId;
					}
					NodeData.Outputs[OutIdx].NextNodeId = NewNext;
				}
			}
		}
	}

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
		if (InPin) InPin->PinFriendlyName = FText::FromString(TEXT("From"));

		for (int32 i = 0; i < NodeData.Outputs.Num(); i++)
		{
			const FName PinName(*FString::Printf(TEXT("Out_%d"), i));
			UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, PinName);

			if (OutPin)
			{
				OutPin->PinFriendlyName = NodeData.Outputs[i].Text.IsEmpty()
					? FText::FromString(TEXT("End"))
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