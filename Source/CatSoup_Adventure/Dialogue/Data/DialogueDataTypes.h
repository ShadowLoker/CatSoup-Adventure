// Dialogue data types. Behavior from output count: 0=end, 1=line only, 2+=choices.
// Map key = node id used in "Next Node" links.
#pragma once

/** One outgoing connection from a node. Represents a path the player can take. */
#include "CoreMinimal.h"
#include "DialogueDataTypes.generated.h"

class UDialogueAction;

USTRUCT(BlueprintType)
struct FDialogueOutput
{
	GENERATED_BODY()

	/** Label shown to the player (e.g. "Yes", "No"). For 1-output "continue" flow, often empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Label"))
	FText Text;

	/** Map key of the node to jump to when this output is chosen. Empty = end dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Next Node"))
	FName NextNodeId;

	/** True when this output is wired in the graph. Unwired outputs are disabled and not shown at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEnabled = true;

	/** Actions to run when this choice leads to an End node. Compiled from the connected End node's Actions. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly)
	TArray<TObjectPtr<UDialogueAction>> EndActions;

	/** When connected to an End node, its EndNodeId. Used to look up "next start" entry point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName ConnectedEndNodeId;
};

/** The line of text to display. */
USTRUCT(BlueprintType)
struct FDialogueNode
{
	GENERATED_BODY()
	/** Outgoing connections. Count determines behavior: 0=end, 1=continue, 2+=choices. */

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SpeakerId;

	/** Data passed to UI when a line is shown (1-output case). UI displays this, then calls Advance() when ready. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Outputs"))
	TArray<FDialogueOutput> Outputs;

	/** Actions to execute when this node is entered. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, meta = (DisplayName = "Actions"))
	TArray<TObjectPtr<UDialogueAction>> Actions;
};

/** One choice option. Index is passed to Advance(Index) when clicked. */
USTRUCT(BlueprintType)
struct FDialogueChoicePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Index = 0;

	UPROPERTY(BlueprintReadOnly)
	FText Text;
};

/** Payload for every line. Choices empty = continue flow; Choices non-empty = show choices. */
USTRUCT(BlueprintType)
struct FDialoguePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName SpeakerId;

	UPROPERTY(BlueprintReadOnly)
	FText LineText;

	UPROPERTY(BlueprintReadOnly)
	TArray<FDialogueChoicePayload> Choices;
};
