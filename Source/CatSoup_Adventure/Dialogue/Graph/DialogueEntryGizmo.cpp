// DialogueEntryGizmo.cpp
#include "DialogueEntryGizmo.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

#if WITH_EDITOR
void UDialogueEntryGizmo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UDialogueEntryGizmo, EntryPointId))
	{
		if (EntryPointId.ToString().Equals(TEXT("Default"), ESearchCase::IgnoreCase))
		{
			Modify();
			EntryPointId = NAME_None;
			if (UEdGraph* Graph = GetGraph()) Graph->NotifyGraphChanged();
		}
	}
}
#endif

void UDialogueEntryGizmo::AllocateDefaultPins()
{
	// Input: connect from End node's "NextStart" to set "when you exit via that End, next time start here"
	UEdGraphPin* InPin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("NextStart")));
	if (InPin) InPin->PinFriendlyName = FText::FromString(TEXT(""));

	UEdGraphPin* OutPin = CreatePin(EGPD_Output, DialogueGraphPins::Flow, FName(TEXT("Out")));
	if (OutPin) OutPin->PinFriendlyName = FText::FromString(TEXT(""));
}

void UDialogueEntryGizmo::ReconstructNode()
{
	// Ensure NextStart input pin exists (for legacy nodes saved before it was added)
	if (!FindPin(TEXT("NextStart")))
	{
		Modify();
		UEdGraphPin* Pin = CreatePin(EGPD_Input, DialogueGraphPins::Flow, FName(TEXT("NextStart")));
		if (Pin) Pin->PinFriendlyName = FText::FromString(TEXT(""));
		if (UEdGraph* Graph = GetGraph()) Graph->NotifyGraphChanged();
	}
}

FText UDialogueEntryGizmo::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return EntryPointId.IsNone()
		? FText::FromString(TEXT("Entry (set ID)"))
		: FText::FromName(EntryPointId);
}
