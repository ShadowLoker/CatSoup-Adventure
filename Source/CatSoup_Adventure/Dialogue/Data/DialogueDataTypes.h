// Dialogue data types. Behavior from output count: 0=end, 1=line only, 2+=choices.
// Map key = node id used in "Next Node" links.
#pragma once

/** One outgoing connection from a node. Represents a path the player can take. */
#include "CoreMinimal.h"
#include "DialogueDataTypes.generated.h"

USTRUCT(BlueprintType)
	/** Label shown to the player (e.g. "Yes", "No"). For 1-output "continue" flow, often empty. */
struct FDialogueOutput
{
	/** Map key of the node to jump to when this output is chosen. Empty = end dialogue. */
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Label"))
/** One step in the dialogue. Identified by its map key in UDialogueAsset::Nodes. */
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Next Node"))
	FName NextNodeId;
	/** Who is speaking (e.g. "NPC", "Player"). UI uses this for portrait/name. */
};

	/** The line of text to display. */
USTRUCT(BlueprintType)
struct FDialogueNode
	/** Outgoing connections. Count determines behavior: 0=end, 1=continue, 2+=choices. */
{
	GENERATED_BODY()
	/** Event names to broadcast when this node is entered. Game systems can listen (quests, animations, etc.). */

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName SpeakerId;
/** Data passed to UI when a line is shown (1-output case). UI displays this, then calls Advance() when ready. */

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Outputs"))
	TArray<FDialogueOutput> Outputs;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Events"))
	TArray<FName> EventNames;
};

/** One choice option when choices are shown (2+ outputs). Index is passed to Advance(Index) when clicked. */
USTRUCT(BlueprintType)
struct FDialogueLinePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName SpeakerId;

	UPROPERTY(BlueprintReadOnly)
	FText LineText;
};

USTRUCT(BlueprintType)
struct FDialogueChoicePayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 Index = 0;

	UPROPERTY(BlueprintReadOnly)
	FText Text;
};

/** Full payload for choice nodes (2+ outputs). Line + speaker + choice options. */
USTRUCT(BlueprintType)
struct FDialogueChoicesPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName SpeakerId;

	UPROPERTY(BlueprintReadOnly)
	FText LineText;

	UPROPERTY(BlueprintReadOnly)
	TArray<FDialogueChoicePayload> Choices;
};
