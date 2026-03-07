#include "DialogueGraphNodeFactory.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueStartGizmo.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEntryGizmo.h"
#include "SDialogueGraphNode.h"
#include "SDialogueStartGizmo.h"
#include "SDialogueEndGizmo.h"
#include "SDialogueEntryGizmo.h"

TSharedPtr<SGraphNode> FDialogueGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UDialogueStartGizmo* Gizmo = Cast<UDialogueStartGizmo>(Node))
	{
		return SNew(SDialogueStartGizmo, Gizmo);
	}
	if (UDialogueEndGizmo* EndGizmo = Cast<UDialogueEndGizmo>(Node))
	{
		return SNew(SDialogueEndGizmo, EndGizmo);
	}
	if (UDialogueEntryGizmo* EntryGizmo = Cast<UDialogueEntryGizmo>(Node))
	{
		return SNew(SDialogueEntryGizmo, EntryGizmo);
	}
	if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
	{
		return SNew(SDialogueGraphNode, DNode);
	}
	return nullptr;
}