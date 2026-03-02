// CatSoup Adventure - Dialogue System
#include "GameFramework/Actor.h"
// ====================================
// UDialogueComponent: entry point for starting dialogue. Creates context and session, then starts the asset.
//
#include "Dialogue/Runtime/DialogueComponent.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Runtime/DialogueSession.h"
#include "GameFramework/Actor.h"

UDialogueComponent::UDialogueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogueComponent::BeginDialogue(AActor* Interactor)
{
	if (!DialogueAsset || !DialogueAsset->IsValid()) return;
	EndDialogue(); // tanquem anteriors diàlegs si n'hi ha per a no repetir

	ActiveSession = NewObject<UDialogueSession>(this); // Creem un de nou
	//ActiveSession->Start(DialogueAsset); Iniciarem el dialeg desde un blueprint
}

void UDialogueComponent::EndDialogue()
{
	if (ActiveSession)
	{
		ActiveSession->End();
		ActiveSession = nullptr;
	}
}
