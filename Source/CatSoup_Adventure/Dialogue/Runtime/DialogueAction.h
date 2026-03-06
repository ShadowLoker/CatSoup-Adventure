#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DialogueAction.generated.h"

class UDialogueSession;

/**
 * Blueprint action object triggered by dialogue nodes/endings.
 * Create Blueprint subclasses to implement gameplay logic without string event names.
 */
UCLASS(BlueprintType, Blueprintable, Abstract)
class CATSOUP_ADVENTURE_API UDialogueAction : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void Execute(UDialogueSession* Session);
};

