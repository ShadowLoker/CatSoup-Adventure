// CatSoup Adventure - Dialogue System
// ====================================
// UDialogueSession: traverses nodes and broadcasts delegates. UI/game systems bind to delegates and call Advance().
//
#include "Dialogue/Runtime/DialogueSession.h"
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Runtime/DialogueAction.h"

void UDialogueSession::Start(UDialogueAsset* InAsset, FName InEntryPointId)
{
	if (!InAsset || !InAsset->IsValid()) return;
	End();

	Asset = InAsset;

	// Use entry point if provided and found; otherwise default start
	if (!InEntryPointId.IsNone())
	{
		if (InAsset->EntryPoints.Contains(InEntryPointId))
		{
			CurrentNodeId = InAsset->EntryPoints.FindChecked(InEntryPointId);
			UE_LOG(LogTemp, Log,
				TEXT("DialogueSession::Start using entry point '%s' -> node '%s'"),
				*InEntryPointId.ToString(),
				*CurrentNodeId.ToString());
		}
		else
		{
			CurrentNodeId = InAsset->StartNodeId;
			UE_LOG(LogTemp, Warning,
				TEXT("DialogueSession::Start: entry point '%s' not found in asset '%s'; falling back to StartNodeId '%s'"),
				*InEntryPointId.ToString(),
				*InAsset->GetName(),
				*CurrentNodeId.ToString());
		}
	}
	else
	{
		CurrentNodeId = InAsset->StartNodeId;
		UE_LOG(LogTemp, Log,
			TEXT("DialogueSession::Start using default StartNodeId '%s'"),
			*CurrentNodeId.ToString());
	}

	bIsRunning = true;
	ProcessCurrentNode();
}

void UDialogueSession::ProcessCurrentNode()
{
	if (!Asset || !bIsRunning)
	{
		return;
	}

	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId);
	if (!Node)
	{
		UE_LOG(LogTemp, Error, TEXT("Node not found: %s"), *CurrentNodeId.ToString());
		End();
		return;
	}

	TriggerActions(Node->Actions);

	FDialoguePayload Payload;
	Payload.SpeakerId = Node->SpeakerId;
	Payload.LineText = Node->Text;
	const int32 NumOutputs = Node->Outputs.Num();
	if (NumOutputs >= 1)
	{
		for (int32 i = 0; i < NumOutputs; ++i)
		{
			// Only include wired choices (unwired = disabled, not applied)
			if (Node->Outputs[i].bEnabled)
			{
				Payload.Choices.Add({ i, Node->Outputs[i].Text });
			}
		}
	}
	OnLineStarted.Broadcast(Payload);
}

void UDialogueSession::Advance(int32 OutputIndex)
{
	if (!Asset || !bIsRunning) return;
	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId);
	if (!Node || !Node->Outputs.IsValidIndex(OutputIndex))
	{
		End();
		return;
	}

	const FDialogueOutput& Out = Node->Outputs[OutputIndex];

	// If this choice leads to End, fire actions and set "next start" if wired
	if (Out.NextNodeId.IsNone())
	{
		TriggerActions(Out.EndActions);
		NextEntryPointIdForNextStart = NAME_None;
		if (!Out.ConnectedEndNodeId.IsNone() && Asset->EndToNextEntry.Contains(Out.ConnectedEndNodeId))
		{
			NextEntryPointIdForNextStart = Asset->EndToNextEntry.FindChecked(Out.ConnectedEndNodeId);
		}
		End();
		return;
	}

	GoToNode(Out.NextNodeId);
}

void UDialogueSession::GoToNode(FName NodeId)
{
	if (NodeId.IsNone()) { End(); return; }
	CurrentNodeId = NodeId;
	ProcessCurrentNode();
}

void UDialogueSession::End()
{
	bIsRunning = false;
	OnDialogueEnded.Broadcast();
	// NextEntryPointIdForNextStart stays set until read - component reads it in OnDialogueEnded handler
}

void UDialogueSession::TriggerActions(const TArray<TSubclassOf<UDialogueAction>>& Actions)
{
	for (TSubclassOf<UDialogueAction> ActionClass : Actions)
	{
		if (!ActionClass)
		{
			continue;
		}

		UDialogueAction* Action = NewObject<UDialogueAction>(this, ActionClass);
		if (!Action)
		{
			continue;
		}

		Action->Execute(this);
		OnActionTriggered.Broadcast(Action);

		// Backward compatibility only.
		OnDialogueEvent.Broadcast(ActionClass->GetFName());
	}
}

