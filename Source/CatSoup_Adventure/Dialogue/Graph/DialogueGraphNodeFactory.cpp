#include "DialogueGraphNodeFactory.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueStartGizmo.h"
#include "SDialogueGraphNode.h"
#include "SDialogueStartGizmo.h"

TSharedPtr<SGraphNode> FDialogueGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UDialogueStartGizmo* Gizmo = Cast<UDialogueStartGizmo>(Node))
	{
		return SNew(SDialogueStartGizmo, Gizmo);
	}
	if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
	{
		return SNew(SDialogueGraphNode, DNode);
	}
	return nullptr;
}