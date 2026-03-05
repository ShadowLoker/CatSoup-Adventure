// DialogueEndGizmo.cpp
#include "DialogueEndGizmo.h"

void UDialogueEndGizmo::AllocateDefaultPins()
{
	UEdGraphPin* InPin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("In")));
	if (InPin) InPin->PinFriendlyName = FText::FromString(TEXT(""));

	// Output: connect to Entry Point to set "next time start here"
	UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("NextStart")));
	if (OutPin) OutPin->PinFriendlyName = FText::FromString(TEXT(""));
}

void UDialogueEndGizmo::ReconstructNode()
{
	// Ensure NextStart output pin exists (for legacy nodes saved before it was added)
	if (!FindPin(TEXT("NextStart")))
	{
		Modify();
		UEdGraphPin* Pin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("NextStart")));
		if (Pin) Pin->PinFriendlyName = FText::FromString(TEXT(""));
		if (UEdGraph* Graph = GetGraph()) Graph->NotifyGraphChanged();
	}
}

FText UDialogueEndGizmo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("End"));
}
