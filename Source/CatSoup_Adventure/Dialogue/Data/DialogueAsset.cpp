// CatSoup Adventure - Dialogue System
#include "Dialogue/Data/DialogueAsset.h"
#include "Dialogue/Graph/DialogueGraph.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
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

    // --- 1) Find Start node ---
    UDialogueGraphNode* StartNode = nullptr;

    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node))
        {
            if (DNode->NodeType == EDialogueGraphNodeType::Start)
            {
                StartNode = DNode;
                break;
            }
        }
    }

    if (!StartNode)
    {
        UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: No Start node found"));
    }
    else
    {
        // --- 2) StartNodeId = NodeId of node connected from Start.Out_0 ---
        UEdGraphPin* StartOutPin = nullptr;

        for (UEdGraphPin* Pin : StartNode->Pins)
        {
            if (Pin && Pin->Direction == EGPD_Output && Pin->PinName == FName(TEXT("Out_0")))
            {
                StartOutPin = Pin;
                break;
            }
        }

        if (!StartOutPin)
        {
            UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Start node has no Out_0 pin"));
        }
        else if (StartOutPin->LinkedTo.Num() == 0 || !StartOutPin->LinkedTo[0])
        {
            UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Start node is not connected to any dialogue node"));
        }
        else
        {
            UEdGraphNode* TargetUE = StartOutPin->LinkedTo[0]->GetOwningNode();
            UDialogueGraphNode* Target = Cast<UDialogueGraphNode>(TargetUE);

            if (!Target)
            {
                UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Start node connected to non-dialogue node"));
            }
            else if (Target->NodeType == EDialogueGraphNodeType::Start)
            {
                UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Start node connected to Start node (invalid)"));
            }
            else
            {
                StartNodeId = Target->NodeId;
            }
        }
    }

    // --- 3) Build runtime Nodes map from all Dialogue nodes ---
    for (UEdGraphNode* Node : EditorGraph->Nodes)
    {
        UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node);
        if (!DNode)
        {
            if (Node)
            {
                UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Non-dialogue node found: %s"), *Node->GetName());
            }
            continue;
        }

        if (DNode->NodeType == EDialogueGraphNodeType::Start)
        {
            continue; // runtime map stores only dialogue nodes
        }

        if (DNode->NodeId.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("CompileFromGraph: Dialogue node with missing NodeId: %s"), *DNode->GetName());
            continue;
        }

        FDialogueNode CompiledData = DNode->NodeData;

        // Clear all next links first (so stale data doesn't survive)
        for (auto& Out : CompiledData.Outputs)
        {
            Out.NextNodeId = NAME_None;
        }

        // Read pin links and fill NextNodeId per output index
        for (UEdGraphPin* Pin : DNode->Pins)
        {
            if (!Pin || Pin->Direction != EGPD_Output)
            {
                continue;
            }

            int32 OutIndex = INDEX_NONE;
            if (!TryParseOutIndex(Pin->PinName, OutIndex))
            {
                // Ignore non "Out_X" pins
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

            if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0])
            {
                if (UDialogueGraphNode* Target = Cast<UDialogueGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
                {
                    if (Target->NodeType != EDialogueGraphNodeType::Start)
                    {
                        NextId = Target->NodeId;
                    }
                }
            }

            CompiledData.Outputs[OutIndex].NextNodeId = NextId;
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
