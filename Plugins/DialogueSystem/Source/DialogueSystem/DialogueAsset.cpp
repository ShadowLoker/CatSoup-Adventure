// CatSoup Adventure - Dialogue System
#include "DialogueAsset.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"

// Define the static delegate
UDialogueAsset::FOnCompileDialogueAsset UDialogueAsset::OnCompileDialogueAsset;

void UDialogueAsset::PreSave(FObjectPreSaveContext SaveContext)
{
	OnCompileDialogueAsset.Broadcast(this);
	Super::PreSave(SaveContext);
}
#endif

bool UDialogueAsset::IsValid() const
{
	return !StartNodeId.IsNone() && Nodes.Contains(StartNodeId);
}
