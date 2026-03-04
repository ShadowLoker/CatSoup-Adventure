#include "DialogueGraphNodeFactory.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "SDialogueGraphNode.h"

TSharedPtr<SGraphNode> FDialogueGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
	{
		return SNew(SDialogueGraphNode, DNode);
	}
	return nullptr;
}