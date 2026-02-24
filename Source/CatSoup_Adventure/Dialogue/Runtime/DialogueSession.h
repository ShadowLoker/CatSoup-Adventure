// CatSoup Adventure - Dialogue System
// UDialogueSession: runtime state machine. Broadcasts delegates; UI/game systems bind and call Advance().
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Dialogue/Data/DialogueDataTypes.h"
#include "DialogueSession.generated.h"

class UDialogueAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueLineStarted, const FDialogueLinePayload&, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueChoicesPresented, const TArray<FDialogueChoicePayload>&, Choices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEvent, FName, EventName);

UCLASS(BlueprintType)
class CATSOUP_ADVENTURE_API UDialogueSession : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnDialogueLineStarted OnLineStarted;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueChoicesPresented OnChoicesPresented;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueEnded OnDialogueEnded;

	UPROPERTY(BlueprintAssignable)
	FOnDialogueEvent OnDialogueEvent;

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Start(UDialogueAsset* Asset);

	UFUNCTION(BlueprintCallable, Category = "Dialogue", meta = (DisplayName = "Advance"))
	void Advance(int32 OutputIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void End();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsRunning() const { return bIsRunning; }

private:
	UPROPERTY()
	TObjectPtr<UDialogueAsset> Asset;

	FName CurrentNodeId;
	bool bIsRunning = false;

	void ProcessCurrentNode();
	void GoToNode(FName NodeId);
};
