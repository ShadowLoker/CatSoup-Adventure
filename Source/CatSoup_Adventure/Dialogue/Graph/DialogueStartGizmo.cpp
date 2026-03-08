// DialogueStartGizmo.cpp
#if WITH_EDITOR
#include "DialogueStartGizmo.h"

void UDialogueStartGizmo::AllocateDefaultPins()
{
	UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("Out")));
	if (OutPin)
	{
		OutPin->PinFriendlyName = FText::FromString(TEXT(""));
	}
}

FText UDialogueStartGizmo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Start"));
}

#endif
