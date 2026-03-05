// Attach to NPCs. Holds dialogue asset; creates session on BeginDialogue.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogueComponent.generated.h"

class UDialogueAsset;
class UDialogueSession;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class CATSOUP_ADVENTURE_API UDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

	/** The dialogue to run. Set in the editor or via Blueprint. */
public:
	UDialogueComponent();
	/** Active session after BeginDialogue. Bind to its delegates for UI/game logic. */

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UDialogueAsset> DialogueAsset;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UDialogueSession> ActiveSession;

	/** Set automatically when dialogue ends via an End node with "NextStart" wired. Next BeginDialogue uses this entry point. */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName PendingNextEntryPointId;
	
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FName EntryPointId = NAME_None; 

	/** Start dialogue. No override: uses graph logic (PendingNextEntryPointId if set from last exit, else default). Override "Default": force default start. Override "Return" etc: force that entry point. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginDialogue(AActor* Interactor, FName EntryPointOverride = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();
};
