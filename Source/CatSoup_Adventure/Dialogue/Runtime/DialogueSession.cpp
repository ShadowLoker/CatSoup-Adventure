// CatSoup Adventure - Dialogue System
// ====================================
// UDialogueSession: traverses nodes and broadcasts delegates. UI/game systems bind to delegates and call Advance().
//
#include "Dialogue/Runtime/DialogueSession.h"
#include "Dialogue/Data/DialogueAsset.h"

void UDialogueSession::Start(UDialogueAsset* InAsset)
{
	if (!InAsset || !InAsset->IsValid()) return;
	End();

	Asset = InAsset;
	CurrentNodeId = Asset->StartNodeId;
	bIsRunning = true;
	ProcessCurrentNode();
}

void UDialogueSession::ProcessCurrentNode()
{
	if (!Asset || !bIsRunning) return;

	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId);
	if (!Node) { End(); return; }

	for (const FName& EventName : Node->EventNames)
	{
		if (!EventName.IsNone()) OnDialogueEvent.Broadcast(EventName);
	}

	const int32 NumOutputs = Node->Outputs.Num();
	if (NumOutputs == 0) { End(); return; }

	FDialoguePayload Payload;
	Payload.SpeakerId = Node->SpeakerId;
	Payload.LineText = Node->Text;
	if (NumOutputs >= 2)
	{
		for (int32 i = 0; i < NumOutputs; ++i)
		{
			Payload.Choices.Add({ i, Node->Outputs[i].Text });
		}
	}
	OnLineStarted.Broadcast(Payload);
}

void UDialogueSession::Advance(int32 OutputIndex)
{
	if (!Asset || !bIsRunning) return;
	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId); 
	if (!Node || !Node->Outputs.IsValidIndex(OutputIndex)) return;
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
