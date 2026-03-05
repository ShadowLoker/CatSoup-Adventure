// DialogueEndGizmo.cpp
#include "DialogueEndGizmo.h"

void UDialogueEndGizmo::AllocateDefaultPins()
{
	UEdGraphPin* InPin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("In")));
	if (InPin)
	{
		InPin->PinFriendlyName = FText::FromString(TEXT(""));
	}
}

FText UDialogueEndGizmo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("End"));
}
