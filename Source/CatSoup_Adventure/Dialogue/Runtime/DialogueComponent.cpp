// CatSoup Adventure - Dialogue System
#include "Dialogue/Runtime/DialogueComponent.h"
#include "GameFramework/Actor.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Runtime/DialogueSession.h"
#include "Dialogue/Runtime/DialogueAction.h"

UDialogueComponent::UDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogueComponent::BeginDialogue(AActor* Interactor, FName EntryPointOverride)
{
	UE_LOG(LogTemp, Log, TEXT("DialogueComponent::BeginDialogue called on %s, Interactor: %s"), *GetOwner()->GetName(), Interactor ? *Interactor->GetName() : TEXT("None"));
	if (!DialogueAsset || !DialogueAsset->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DialogueComponent on %s has no valid DialogueAsset assigned"), *GetOwner()->GetName());
		return;
	}
	EndDialogue();

	EntryPointId = NAME_None;

	if (EntryPointOverride.IsNone())
	{
		// No override: use graph logic (pending from last exit, else default)
		EntryPointId = PendingNextEntryPointId;
	}
	else if (EntryPointOverride == FName(TEXT("Default")) || EntryPointOverride == FName(TEXT("DEFAULT")) || EntryPointOverride == FName(TEXT("DEAFULT")))
	{
		// Explicit "Default": force default start
		EntryPointId = NAME_None;
	}
	else
	{
		// Override with specific entry point ID
		EntryPointId = EntryPointOverride;
	}

	PendingNextEntryPointId = NAME_None; // Use once, then clear

	ActiveSession = NewObject<UDialogueSession>(this);
	ActiveSession->OnActionTriggered.AddDynamic(this, &UDialogueComponent::HandleSessionAction);
	ActiveSession->Start(DialogueAsset, EntryPointId);
	UE_LOG(LogTemp, Log, TEXT("Dialogue session started with asset: %s, entry: %s"), *DialogueAsset->GetName(), *EntryPointId.ToString());
}

void UDialogueComponent::EndDialogue()
{
	if (ActiveSession)
	{
		PendingNextEntryPointId = ActiveSession->GetNextEntryPointId();
		ActiveSession->End();
		ActiveSession = nullptr;
	}
}

void UDialogueComponent::HandleSessionAction(UDialogueAction* Action)
{
	OnActionTriggered.Broadcast(Action);
}
