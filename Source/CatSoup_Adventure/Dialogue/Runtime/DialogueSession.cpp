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
	CurrentNodeId = Asset->StartNodeId; //anem al startnode
	bIsRunning = true;
	ProcessCurrentNode();
}

void UDialogueSession::ProcessCurrentNode()
{
	if (!Asset || !bIsRunning) return;

	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId); //El busquem al map (no m'acaba)
	if (!Node) { End(); return; }

	for (const FName& EventName : Node->EventNames) //LLançem els events només començar
	{
		if (!EventName.IsNone()) OnDialogueEvent.Broadcast(EventName);
	}

	const int32 NumOutputs = Node->Outputs.Num(); //mirem quants outputs tenim
	if (NumOutputs == 0) { End(); return; } // si no hi ha, endnode

	if (NumOutputs == 1) //si hi ha 1 és un fil
	{
		FDialogueLinePayload Payload;
		Payload.SpeakerId = Node->SpeakerId;
		Payload.LineText = Node->Text;
		OnLineStarted.Broadcast(Payload);
		return;
	}

	FDialogueChoicesPayload Payload;
	Payload.SpeakerId = Node->SpeakerId;
	Payload.LineText = Node->Text;
	for (int32 i = 0; i < NumOutputs; ++i)
	{
		FDialogueChoicePayload P;
		P.Index = i;
		P.Text = Node->Outputs[i].Text;
		Payload.Choices.Add(P);
	}
	OnChoicesPresented.Broadcast(Payload);
	
}

void UDialogueSession::Advance(int32 OutputIndex) //Busquem el segûent al mapa
{
	if (!Asset || !bIsRunning) return;
	const FDialogueNode* Node = Asset->Nodes.Find(CurrentNodeId); 
	if (!Node || !Node->Outputs.IsValidIndex(OutputIndex)) return;
	GoToNode(Node->Outputs[OutputIndex].NextNodeId);
}

void UDialogueSession::GoToNode(FName NodeId) //el processem
{
	if (NodeId.IsNone()) { End(); return; }
	CurrentNodeId = NodeId;
	ProcessCurrentNode();
}

void UDialogueSession::End() //acabem dialeg (Enviem a UI tmb)
{
	bIsRunning = false;
	OnDialogueEnded.Broadcast();
}
