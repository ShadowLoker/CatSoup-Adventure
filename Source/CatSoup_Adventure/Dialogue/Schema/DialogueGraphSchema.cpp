#include "DialogueGraphSchema.h"
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueGraphPins.h"
#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"


#define LOCTEXT_NAMESPACE "DialogueGraphSchema"

namespace DialogueSchemaActions
{
    struct FDialogueNewNodeAction : public FEdGraphSchemaAction
    {
        EDialogueGraphNodeType Type;

        FDialogueNewNodeAction(
            const FText& InCategory,
            const FText& InMenuDesc,
            const FText& InToolTip,
            int32 InGrouping,
            EDialogueGraphNodeType InType)
            : FEdGraphSchemaAction(InCategory, InMenuDesc, InToolTip, InGrouping)
            , Type(InType)
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

            // Enforce single Start node
            if (Type == EDialogueGraphNodeType::Start)
            {
                for (UEdGraphNode* Node : ParentGraph->Nodes)
                {
                    const UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(Node);
                    if (DNode && DNode->NodeType == EDialogueGraphNodeType::Start)
                    {
                        return nullptr;
                    }
                }
            }

            const FScopedTransaction Transaction(
                NSLOCTEXT("DialogueGraph", "AddDialogueNode", "Add Dialogue Node"));

            ParentGraph->Modify();

            // Create node (Outer = graph)
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
            NewNode->NodeType = Type;

            // Stable id
            if (NewNode->NodeId.IsNone())
            {
                NewNode->NodeId = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
            }

            // Defaults
            if (Type == EDialogueGraphNodeType::Dialogue)
            {
                if (NewNode->NodeData.Text.IsEmpty())
                {
                    NewNode->NodeData.Text = FText::FromString(TEXT("..."));
                }

                if (NewNode->NodeData.Outputs.Num() == 0)
                {
                    NewNode->NodeData.Outputs.AddDefaulted();
                    NewNode->NodeData.Outputs[0].Text = FText::FromString(TEXT("Next"));
                }
            }
            else if (Type == EDialogueGraphNodeType::Start)
            {
                // Start node doesn't need dialogue outputs data (pins are handled by AllocateDefaultPins)
                NewNode->NodeData.Outputs.Empty();
            }

            // Position
            NewNode->NodePosX = (int32)Location.X;
            NewNode->NodePosY = (int32)Location.Y;

            // Add + build pins
            ParentGraph->AddNode(NewNode, /*bFromUI=*/true, bSelectNewNode);
            NewNode->AllocateDefaultPins();

            // If created from an existing pin, try to connect it
            NewNode->AutowireNewNode(FromPin);

            // Notify editor
            NewNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return NewNode;
        }
    };
}

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    const FText Category = LOCTEXT("DialogueCategory", "Dialogue");

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddStartNode", "Add Start Node"),
        LOCTEXT("AddStartNodeTooltip", "Creates the start node of the dialogue."),
        0,
        EDialogueGraphNodeType::Start));

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddDialogueNode", "Add Dialogue Node"),
        LOCTEXT("AddDialogueNodeTooltip", "Creates a dialogue node."),
        0,
        EDialogueGraphNodeType::Dialogue));
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

    // Start.Out_0: single connection (replace)
    if (const UDialogueGraphNode* OutNode = Cast<UDialogueGraphNode>(OutPin->GetOwningNode()))
    {
        if (OutNode->NodeType == EDialogueGraphNodeType::Start && OutPin->LinkedTo.Num() > 0)
            return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Replace Start connection"));
    }

    // Each output option should be single connection (replace existing)
    if (OutPin->LinkedTo.Num() > 0)
        return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Replace existing option link"));

    // INPUT: allow multiple connections (no break-others)
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
        const UEdGraphPin* PinConst = Context->Pin;           // likely const UEdGraphPin*
        UEdGraphNode* OwnerNode = PinConst ? const_cast<UEdGraphNode*>(PinConst->GetOwningNode()) : nullptr;
        const FName PinName = PinConst ? PinConst->PinName : NAME_None;

        TWeakObjectPtr<UEdGraphNode> WeakOwnerNode = OwnerNode;

        Section.AddMenuEntry(
            "DialogueBreakPinLinks",
            FText::FromString("Break Links"),
            FText::FromString("Break all links from this pin"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([WeakOwnerNode, PinName]()
            {
                UEdGraphNode* Node = WeakOwnerNode.Get();
                if (!Node || PinName.IsNone())
                    return;

                UEdGraph* Graph = Node->GetGraph();

                const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "BreakPinLinks", "Break Pin Links"));
                if (Graph) Graph->Modify();
                Node->Modify();

                // Find the pin again by name
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
        return;
    }

    // Right-click on a NODE
    if (Context->Node)
    {
        FToolMenuSection& Section = Menu->AddSection("DialogueNode", FText::FromString("Node"));
        TWeakObjectPtr<const UEdGraphNode> WeakNode = Context->Node; // TObjectPtr<const UEdGraphNode>

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

static bool TryParseOutIndex(const FName& PinName, int32& OutIndex)
{
    const FString S = PinName.ToString();
    if (!S.StartsWith(TEXT("Out_"))) return false;
    const FString Right = S.Mid(4);
    if (!Right.IsNumeric()) return false;
    OutIndex = FCString::Atoi(*Right);
    return OutIndex >= 0;
}

static void UpdateNextNodeFromPinLink(UEdGraphPin* OutPin)
{
    if (!OutPin || OutPin->Direction != EGPD_Output) return;

    UDialogueGraphNode* FromNode = Cast<UDialogueGraphNode>(OutPin->GetOwningNode());
    if (!FromNode || FromNode->NodeType == EDialogueGraphNodeType::Start) return;

    int32 OutIndex = INDEX_NONE;
    if (!TryParseOutIndex(OutPin->PinName, OutIndex)) return;
    if (!FromNode->NodeData.Outputs.IsValidIndex(OutIndex)) return;

    FromNode->Modify();

    FName NewNext = NAME_None;
    if (OutPin->LinkedTo.Num() > 0 && OutPin->LinkedTo[0])
    {
        if (UDialogueGraphNode* ToNode = Cast<UDialogueGraphNode>(OutPin->LinkedTo[0]->GetOwningNode()))
        {
            if (ToNode->NodeType != EDialogueGraphNodeType::Start)
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
