// CatSoup Adventure - Dialogue System
#include "Dialogue/Runtime/DialogueComponent.h"
#include "GameFramework/Actor.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Runtime/DialogueSession.h"
#include "GameFramework/Actor.h"

UDialogueComponent::UDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogueComponent::BeginDialogue(AActor* Interactor)
{
	UE_LOG(LogTemp, Log, TEXT("DialogueComponent::BeginDialogue called on %s, Interactor: %s"), *GetOwner()->GetName(), Interactor ? *Interactor->GetName() : TEXT("None"));
	if (!DialogueAsset || !DialogueAsset->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DialogueComponent on %s has no valid DialogueAsset assigned"), *GetOwner()->GetName());
		return;
	}
	EndDialogue(); // tanquem anteriors diàlegs si n'hi ha per a no repetir

	ActiveSession = NewObject<UDialogueSession>(this); // Creem un de nou
	//ActiveSession->Start(DialogueAsset); Iniciarem el dialeg desde un blueprint
	UE_LOG(LogTemp, Log, TEXT("Dialogue session started with asset: %s"), *DialogueAsset->GetName());
}

void UDialogueComponent::EndDialogue()
{
	if (ActiveSession)
	{
		ActiveSession->End();
		ActiveSession = nullptr;
	}
}
