// DialogueEntryGizmo.cpp
#include "DialogueEntryGizmo.h"

void UDialogueEntryGizmo::AllocateDefaultPins()
{
	UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("Out")));
	if (OutPin)
	{
		OutPin->PinFriendlyName = FText::FromString(TEXT(""));
	}
}

FText UDialogueEntryGizmo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return EntryPointId.IsNone()
		? FText::FromString(TEXT("Entry (set ID)"))
		: FText::FromName(EntryPointId);
}
