// CatSoup Adventure - Dialogue System
// ====================================
// UDialogueSession: traverses nodes and broadcasts delegates. UI/game systems bind to delegates and call Advance().
//
#include "Dialogue/Runtime/DialogueSession.h"
#include "Dialogue/Data/DialogueAsset.h"

void UDialogueSession::Start(UDialogueAsset* InAsset, FName InEntryPointId)
{
	if (!InAsset || !InAsset->IsValid()) return;
	End();

	Asset = InAsset;

	// Use entry point if provided and found; otherwise default start
	if (!InEntryPointId.IsNone() && InAsset->EntryPoints.Contains(InEntryPointId))
	{
		CurrentNodeId = InAsset->EntryPoints.FindChecked(InEntryPointId);
	}
	else
	{
		CurrentNodeId = InAsset->StartNodeId;
	}

	bIsRunning = true;
	ProcessCurrentNode();
}

void UDialogueSession::ProcessCurrentNode()
{
	UE_LOG(LogTemp, Warning, TEXT("Processing node: %s"), *CurrentNodeId.ToString());
	if (!Asset || !bIsRunning)
	{
		UE_LOG(LogTemp, Warning, TEXT("No asset or session not running"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("Processing node: %s"), *Asset->GetName());

	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId);
	if (!Node)
	{
		UE_LOG(LogTemp, Error, TEXT("Node not found: %s"), *CurrentNodeId.ToString());
		End();
		return;
	}

	for (const FName& EventName : Node->EventNames)
	{
		if (!EventName.IsNone()) OnDialogueEvent.Broadcast(EventName);
	}

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
	UE_LOG(LogTemp, Warning, TEXT("OnLineStarted broadcast - Speaker: %s, Text: %s"), *Node->SpeakerId.ToString(), *Node->Text.ToString());
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
	GoToNode(Node->Outputs[OutputIndex].NextNodeId);
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
}
