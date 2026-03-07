// CatSoup Adventure - Dialogue System
// UDialogueSession: runtime state machine. Broadcasts delegates; UI/game systems bind and call Advance().
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dialogue/Data/DialogueDataTypes.h"
#include "DialogueSession.generated.h"

class UDialogueAsset;
class UDialogueAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueLineStarted, const FDialoguePayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueActionTriggered, UDialogueAction*, Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEvent, FName, EventName);

UCLASS(BlueprintType)
class CATSOUP_ADVENTURE_API UDialogueSession : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnDialogueLineStarted OnLineStarted;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueEnded OnDialogueEnded;

	/** Preferred dispatcher: bind in Blueprints and cast Action to your custom Blueprint action class. */
	UPROPERTY(BlueprintAssignable)
	FOnDialogueActionTriggered OnActionTriggered;

	/** Backward compatibility only. Emits Action class name when actions execute. */
	UPROPERTY(BlueprintAssignable)
	FOnDialogueEvent OnDialogueEvent;

	/** Start dialogue. Use optional EntryPointId (e.g. "Return", "Continue") to begin from an alternate entry point; pass NAME_None for default start. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (DisplayName = "Start"))
	void Start(UDialogueAsset* Asset, FName EntryPointId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (DisplayName = "Advance"))
	void Advance(int32 OutputIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void End();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsRunning() const { return bIsRunning; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FName GetCurrentNodeId() const { return CurrentNodeId; }

	/** When dialogue ended via an End node with "NextStart" wired to an Entry Point, this is that EntryPointId. Read after OnDialogueEnded to use next time. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	FName GetNextEntryPointId() const { return NextEntryPointIdForNextStart; }

private:
	UPROPERTY()
	TObjectPtr<UDialogueAsset> Asset;

	FName CurrentNodeId;
	bool bIsRunning = false;
	FName NextEntryPointIdForNextStart;

	void ProcessCurrentNode();
	void GoToNode(FName NodeId);
	void TriggerActions(const TArray<TObjectPtr<UDialogueAction>>& Actions);
};
