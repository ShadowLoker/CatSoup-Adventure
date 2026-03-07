// CatSoup Adventure - Dialogue System
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Graph/DialogueGraph.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueStartGizmo.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEntryGizmo.h"
#include "Dialogue/Runtime/DialogueAction.h"
#include "UObject/ObjectSaveContext.h"

static bool TryParseOutIndex(const FName& PinName, int32& OutIndex)
{
    // Expected: Out_0, Out_1, ...
    const FString S = PinName.ToString();
    if (!S.StartsWith(TEXT("Out_")))
    {
        return false;
    }

    const FString Right = S.Mid(4);
    if (!Right.IsNumeric())
    {
        return false;
    }

    OutIndex = FCString::Atoi(*Right);
    return OutIndex >= 0;
}

void UDialogueAsset::CompileFromGraph()
{
    Nodes.Empty();
    StartNodeId = NAME_None;

    if (!EditorGraph)
    {
        UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: EditorGraph is null"));
        return;
    }

    auto DuplicateActionsToAsset = [this](const TArray<TObjectPtr<UDialogueAction>>& SourceActions)
    {
        TArray<TObjectPtr<UDialogueAction>> DuplicatedActions;
        DuplicatedActions.Reserve(SourceActions.Num());
        for (UDialogueAction* SourceAction : SourceActions)
        {
            if (!SourceAction)
            {
                continue;
            }

            if (UDialogueAction* Duplicated = DuplicateObject<UDialogueAction>(SourceAction, this))
            {
                DuplicatedActions.Add(Duplicated);
            }
        }
        return DuplicatedActions;
    };

    // --- 1) Find Start Gizmo and get node connected to its output ---
    UDialogueStartGizmo* StartGizmo = nullptr;
    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        if (UDialogueStartGizmo* Gizmo = Cast<UDialogueStartGizmo>(Node))
        {
            StartGizmo = Gizmo;
            break;
        }
    }

    if (StartGizmo)
    {
        for (UEdGraphPin* Pin : StartGizmo->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
            {
                if (UDialogueGraphNode* Target = Cast<UDialogueGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
                {
                    StartNodeId = Target->NodeId;
                    break;
                }
            }
        }
    }

    if (StartNodeId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Start gizmo not connected to a dialogue node"));
    }

    // --- 1b) Build EntryPoints from all Entry gizmos ---
    EntryPoints.Empty();
    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        if (UDialogueEntryGizmo* EntryGizmo = Cast<UDialogueEntryGizmo>(Node))
        {
            if (EntryGizmo->EntryPointId.IsNone()) continue;

            for (UEdGraphPin* Pin : EntryGizmo->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
                {
                    if (UDialogueGraphNode* Target = Cast<UDialogueGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
                    {
                        EntryPoints.Add(EntryGizmo->EntryPointId, Target->NodeId);
                        break;
                    }
                }
            }
        }
    }

    // --- 1c) Build EndToNextEntry: when you exit via End X, next time start at Entry Y ---
    EndToNextEntry.Empty();
    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        if (UDialogueEndGizmo* EndGizmo = Cast<UDialogueEndGizmo>(Node))
        {
            // Use EndNodeId if set; otherwise fallback to NodeGuid so "next start" still works
            FName EndKey = EndGizmo->EndNodeId.IsNone()
                ? FName(*EndGizmo->NodeGuid.ToString())
                : EndGizmo->EndNodeId;

            for (UEdGraphPin* Pin : EndGizmo->Pins)
            {
                if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == FName(TEXT("NextStart")) &&
                    Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
                {
                    if (UDialogueEntryGizmo* EntryGizmo = Cast<UDialogueEntryGizmo>(Pin->LinkedTo[0]->GetOwningNode()))
                    {
                        if (!EntryGizmo->EntryPointId.IsNone())
                        {
                            EndToNextEntry.Add(EndKey, EntryGizmo->EntryPointId);
                        }
                        break;
                    }
                }
            }
        }
    }

    // --- 2) Build runtime Nodes map from all Dialogue nodes ---
    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node);
        if (!DNode)
        {
            continue; // Skip Start Gizmo and other non-dialogue nodes
        }

        if (DNode->NodeId.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Dialogue node with missing NodeId: %s"), *DNode->GetName());
            continue;
        }

        FDialogueNode CompiledData = DNode->NodeData;
        CompiledData.Actions = DuplicateActionsToAsset(DNode->NodeData.Actions);

        // Clear all next links, enabled state, end events, and connected end id
        for (auto& Out : CompiledData.Outputs)
        {
            Out.NextNodeId = NAME_None;
            Out.bEnabled = false;
            Out.EndActions.Empty();
            Out.ConnectedEndNodeId = NAME_None;
        }

        // Read pin links and fill NextNodeId + bEnabled per output index
        for (UEdGraphPin* Pin : DNode->Pins)
        {
            if (!Pin || Pin->Direction != EGPD_Output)
            {
                continue;
            }

            int32 OutIndex = INDEX_NONE;
            if (!TryParseOutIndex(Pin->PinName, OutIndex))
            {
                continue;
            }

            if (!CompiledData.Outputs.IsValidIndex(OutIndex))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("CompileFromGraph: Node %s has pin %s but Outputs[%d] doesn't exist"),
                    *DNode->NodeId.ToString(),
                    *Pin->PinName.ToString(),
                    OutIndex);
                continue;
            }

            FName NextId = NAME_None;
            bool bWired = false;
            TArray<TObjectPtr<UDialogueAction>> EndActs;
            FName ConnectedEndId = NAME_None;

            if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
            {
                UEdGraphNode* Target = Pin->LinkedTo[0]->GetOwningNode();
                if (UDialogueGraphNode* DTarget = Cast<UDialogueGraphNode>(Target))
                {
                    NextId = DTarget->NodeId;
                    bWired = true;
                }
                else if (UDialogueEndGizmo* EndGizmo = Cast<UDialogueEndGizmo>(Target))
                {
                    bWired = true;
                    EndActs = DuplicateActionsToAsset(EndGizmo->Actions);
                    ConnectedEndId = EndGizmo->EndNodeId.IsNone()
                        ? FName(*EndGizmo->NodeGuid.ToString())
                        : EndGizmo->EndNodeId;
                }
            }

            CompiledData.Outputs[OutIndex].NextNodeId = NextId;
            CompiledData.Outputs[OutIndex].bEnabled = bWired;
            CompiledData.Outputs[OutIndex].EndActions = EndActs;
            CompiledData.Outputs[OutIndex].ConnectedEndNodeId = ConnectedEndId;
        }

        Nodes.Add(DNode->NodeId, CompiledData);
    }

    // --- 4) Final warnings ---
    if (StartNodeId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: StartNodeId is None (Start not connected or missing)"));
    }

    if (!StartNodeId.IsNone() && !Nodes.Contains(StartNodeId))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("CompileFromGraph: StartNodeId (%s) is not present in Nodes map"),
            *StartNodeId.ToString());
    }
    Modify();
}

void UDialogueAsset::PreSave(FObjectPreSaveContext SaveContext)
{
	CompileFromGraph();
	Super::PreSave(SaveContext);
}

bool UDialogueAsset::IsValid() const
{
	return !StartNodeId.IsNone() && Nodes.Contains(StartNodeId);
}

