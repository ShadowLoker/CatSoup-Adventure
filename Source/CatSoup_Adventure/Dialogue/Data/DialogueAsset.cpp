// CatSoup Adventure - Dialogue System
#include "Dialogue/Data/DialogueAsset.h"

bool UDialogueAsset::IsValid() const
{
	return !StartNodeId.IsNone() && Nodes.Contains(StartNodeId);
}
