#include "DialogueGraphSchema.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueGraphPins.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"


#define LOCTEXT_NAMESPACE "DialogueGraphSchema"

namespace DialogueSchemaActions
{
    struct FDialogueNewNodeAction : public FEdGraphSchemaAction
    {
        FDialogueNewNodeAction(
            const FText& InCategory,
            const FText& InMenuDesc,
            const FText& InToolTip,
            int32 InGrouping)
            : FEdGraphSchemaAction(InCategory, InMenuDesc, InToolTip, InGrouping)
        {}

        virtual UEdGraphNode* PerformAction(
            UEdGraph* ParentGraph,
            UEdGraphPin* FromPin,
            const FVector2D Location,
            bool bSelectNewNode) override
        {
            if (!ParentGraph)
            {
                return nullptr;
            }

            const FScopedTransaction Transaction(
                NSLOCTEXT("DialogueGraph", "AddDialogueNode", "Add Dialogue Node"));

            ParentGraph->Modify();

            UDialogueGraphNode* NewNode = NewObject<UDialogueGraphNode>(
                ParentGraph,
                UDialogueGraphNode::StaticClass(),
                NAME_None,
                RF_Transactional);

            if (!NewNode)
            {
                return nullptr;
            }

            NewNode->Modify();

            if (NewNode->NodeId.IsNone())
            {
                NewNode->NodeId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
            }

            if (NewNode->NodeData.Text.IsEmpty())
            {
                NewNode->NodeData.Text = FText::FromString(TEXT("..."));
            }

            if (NewNode->NodeData.Outputs.Num() == 0)
            {
                NewNode->NodeData.Outputs.AddDefaulted();
                NewNode->NodeData.Outputs[0].Text = FText::FromString(TEXT("Next"));
            }

            NewNode->NodePosX = (int32)Location.X;
            NewNode->NodePosY = (int32)Location.Y;

            ParentGraph->AddNode(NewNode, /*bFromUI=*/true, bSelectNewNode);
            NewNode->AllocateDefaultPins();

            // If created from a pin (e.g. drag to empty space), auto-connect
            NewNode->AutowireNewNode(FromPin);

            NewNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return NewNode;
        }
    };
}

static bool TryParseOutIndex(const FName& PinName, int32& OutIndex)
{
    const FString S = PinName.ToString();
    if (!S.StartsWith(TEXT("Out_"))) return false;
    const FString Right = S.Mid(4);
    if (!Right.IsNumeric()) return false;
    OutIndex = FCString::Atoi(*Right);
    return OutIndex >= 0;
}

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    const FText Category = LOCTEXT("DialogueCategory", "Dialogue");

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddDialogueNode", "Add Dialogue Node"),
        LOCTEXT("AddDialogueNodeTooltip", "Creates a dialogue node. Connect the Start gizmo to a node's From pin to set where the dialogue begins."),
        0));
}

const FPinConnectionResponse UDialogueGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const
{
    if (!A || !B)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins"));

    if (A->PinType.PinCategory != DialogueGraphPins::Flow ||
        B->PinType.PinCategory != DialogueGraphPins::Flow)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Only dialogue flow connections allowed"));

    if (A->Direction == B->Direction)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins must be opposite directions"));

    const UEdGraphPin* OutPin = (A->Direction == EGPD_Output) ? A : B;
    const UEdGraphPin* InPin  = (A->Direction == EGPD_Input)  ? A : B;

    // Allow all valid flow connections. Both Start and Dialogue outputs accept multiple connections.
    // Runtime uses the first connection (LinkedTo[0]) for the flow path.
    return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

void UDialogueGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
    Super::GetContextMenuActions(Menu, Context);

    if (!Context) return;

    // Right-click on a PIN
    if (Context->Pin)
    {
        FToolMenuSection& Section = Menu->AddSection("DialoguePin", FText::FromString("Pin"));
        const UEdGraphPin* PinConst = Context->Pin;
        UEdGraphNode* PinOwner = PinConst ? const_cast<UEdGraphNode*>(PinConst->GetOwningNode()) : nullptr;
        UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(PinOwner);
        const FName PinName = PinConst ? PinConst->PinName : NAME_None;

        TWeakObjectPtr<UEdGraphNode> WeakPinOwner(PinOwner);

        Section.AddMenuEntry(
            "DialogueBreakPinLinks",
            FText::FromString("Break Links"),
            FText::FromString("Break all links from this pin"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([WeakPinOwner, PinName]()
            {
                UEdGraphNode* Node = WeakPinOwner.Get();
                if (!Node || PinName.IsNone())
                    return;

                UEdGraph* Graph = Node->GetGraph();

                const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "BreakPinLinks", "Break Pin Links"));
                if (Graph) Graph->Modify();
                Node->Modify();

                UEdGraphPin* Pin = nullptr;
                for (UEdGraphPin* P : Node->Pins)
                {
                    if (P && P->PinName == PinName)
                    {
                        Pin = P;
                        break;
                    }
                }

                if (Pin)
                {
                    Pin->BreakAllPinLinks(true);
                }
            }))
        );

        // Remove Output: for output pins of Dialogue nodes (not Gizmo) when more than one output exists
        TWeakObjectPtr<UDialogueGraphNode> WeakDNode(DNode);
        int32 OutIndex = INDEX_NONE;
        if (DNode && PinConst->Direction == EGPD_Output && TryParseOutIndex(PinName, OutIndex) &&
            DNode->NodeData.Outputs.Num() > 1 && DNode->NodeData.Outputs.IsValidIndex(OutIndex))
        {
            Section.AddMenuEntry(
                "DialogueRemoveOutput",
                FText::FromString("Remove Output"),
                FText::FromString("Remove this output option"),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateLambda([WeakDNode, OutIndex]()
                {
                    UDialogueGraphNode* N = WeakDNode.Get();
                    if (!N || !N->NodeData.Outputs.IsValidIndex(OutIndex) || N->NodeData.Outputs.Num() <= 1)
                        return;

                    const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "RemoveOutput", "Remove Output"));
                    if (UEdGraph* G = N->GetGraph()) G->Modify();
                    N->Modify();
                    N->RemovedOutputIndexDuringReconstruct = OutIndex;
                    N->NodeData.Outputs.RemoveAt(OutIndex);
                    N->ReconstructNode();
                    if (UEdGraph* G = N->GetGraph()) G->NotifyGraphChanged();
                }))
            );
        }
        return;
    }

    // Right-click on a NODE
    if (Context->Node)
    {
        FToolMenuSection& Section = Menu->AddSection("DialogueNode", FText::FromString("Node"));
        TWeakObjectPtr<const UEdGraphNode> WeakNode = Context->Node;

        Section.AddMenuEntry(
            "DialogueBreakNodeLinks",
            FText::FromString("Break All Node Links"),
            FText::FromString("Break all links to/from this node"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([WeakNode]()
            {
                const UEdGraphNode* NodeConst = WeakNode.Get();
                if (!NodeConst) return;

                UEdGraphNode* Node = const_cast<UEdGraphNode*>(NodeConst);
                UEdGraph* Graph = Node->GetGraph();

                const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "BreakNodeLinks", "Break Node Links"));
                if (Graph) Graph->Modify();
                Node->Modify();

                Node->BreakAllNodeLinks(true);
            }))
        );
    }
}

static void UpdateNextNodeFromPinLink(UEdGraphPin* OutPin)
{
    if (!OutPin || OutPin->Direction != EGPD_Output) return;

    UDialogueGraphNode* FromNode = Cast<UDialogueGraphNode>(OutPin->GetOwningNode());
    if (!FromNode) return;

    int32 OutIndex = INDEX_NONE;
    if (!TryParseOutIndex(OutPin->PinName, OutIndex)) return;
    if (!FromNode->NodeData.Outputs.IsValidIndex(OutIndex)) return;

    FromNode->Modify();

    FName NewNext = NAME_None;
    if (OutPin->LinkedTo.Num() > 0 && OutPin->LinkedTo[0])
    {
        if (UDialogueGraphNode* ToNode = Cast<UDialogueGraphNode>(OutPin->LinkedTo[0]->GetOwningNode()))
        {
            NewNext = ToNode->NodeId;
        }
    }

    FromNode->NodeData.Outputs[OutIndex].NextNodeId = NewNext;
}

bool UDialogueGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
    const bool bResult = Super::TryCreateConnection(A, B);
    if (!bResult) return false;

    // Normalize to OutPin
    UEdGraphPin* OutPin = (A && A->Direction == EGPD_Output) ? A : B;
    UpdateNextNodeFromPinLink(OutPin);

    if (OutPin && OutPin->GetOwningNode() && OutPin->GetOwningNode()->GetGraph())
        OutPin->GetOwningNode()->GetGraph()->NotifyGraphChanged();

    return true;
}

void UDialogueGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotif) const
{
    // Capture owning node before breaking
    UEdGraphPin* PinPtr = &TargetPin;

    Super::BreakPinLinks(TargetPin, bSendsNodeNotif);

    // If it was an output pin, clear NextNodeId
    if (PinPtr && PinPtr->Direction == EGPD_Output)
    {
        UpdateNextNodeFromPinLink(PinPtr); // now LinkedTo is empty => sets None
        if (UEdGraphNode* Owner = PinPtr->GetOwningNode())
            if (UEdGraph* G = Owner->GetGraph())
                G->NotifyGraphChanged();
    }
}

void UDialogueGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    UEdGraphPin* OutPin = (SourcePin && SourcePin->Direction == EGPD_Output) ? SourcePin :
                          (TargetPin && TargetPin->Direction == EGPD_Output) ? TargetPin : nullptr;

    Super::BreakSinglePinLink(SourcePin, TargetPin);

    if (OutPin)
    {
        UpdateNextNodeFromPinLink(OutPin);
        if (UEdGraphNode* Owner = OutPin->GetOwningNode())
            if (UEdGraph* G = Owner->GetGraph())
                G->NotifyGraphChanged();
    }
}

#undef LOCTEXT_NAMESPACE
