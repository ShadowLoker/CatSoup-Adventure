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

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void BeginDialogue(AActor* Interactor);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();
};
