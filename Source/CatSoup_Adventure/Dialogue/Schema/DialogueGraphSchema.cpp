#include "DialogueGraphSchema.h"

#if WITH_EDITOR
#include "Dialogue/Graph/DialogueGraphNode.h"
#include "Dialogue/Graph/DialogueStartGizmo.h"
#include "Dialogue/Graph/DialogueEndGizmo.h"
#include "Dialogue/Graph/DialogueEntryGizmo.h"
#include "Dialogue/Graph/DialogueGraphPins.h"
#include "Framework/Commands/UIAction.h"
#include "GraphEditor.h"
#include "ScopedTransaction.h"
#include "ToolMenu.h"
#include "ToolMenuDelegates.h"
#include "ToolMenus.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"


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

            // If created from a wire, inherit SPEAKER from the previous node
            if (FromPin)
            {
                if (UDialogueGraphNode* PrevNode = Cast<UDialogueGraphNode>(FromPin->GetOwningNode()))
                {
                    NewNode->NodeData.SpeakerId = PrevNode->NodeData.SpeakerId;
                }
            }

            ParentGraph->AddNode(NewNode, /*bFromUI=*/true, bSelectNewNode);
            NewNode->AllocateDefaultPins();

            // If created from a pin (e.g. drag to empty space), auto-connect
            NewNode->AutowireNewNode(FromPin);

            NewNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return NewNode;
        }
    };

    struct FDialogueAddEndNodeAction : public FEdGraphSchemaAction
    {
        FDialogueAddEndNodeAction(
            const FText& InCategory,
            const FText& InMenuDesc,
            const FText& InToolTip,
            int32 InGrouping)
            : FEdGraphSchemaAction(InCategory, InMenuDesc, InToolTip, InGrouping) {}

        virtual UEdGraphNode* PerformAction(
            UEdGraph* ParentGraph,
            UEdGraphPin* FromPin,
            const FVector2D Location,
            bool bSelectNewNode) override
        {
            if (!ParentGraph) return nullptr;

            const FScopedTransaction Transaction(
                NSLOCTEXT("DialogueGraph", "AddEndNode", "Add End Node"));

            ParentGraph->Modify();

            UDialogueEndGizmo* EndNode = NewObject<UDialogueEndGizmo>(
                ParentGraph,
                UDialogueEndGizmo::StaticClass(),
                NAME_None,
                RF_Transactional);

            if (!EndNode) return nullptr;

            EndNode->Modify();
            EndNode->EndNodeId = FName(TEXT("Leave"));
            EndNode->NodePosX = (int32)Location.X;
            EndNode->NodePosY = (int32)Location.Y;

            ParentGraph->AddNode(EndNode, true, bSelectNewNode);
            EndNode->AllocateDefaultPins();

            if (FromPin && FromPin->Direction == EGPD_Output)
            {
                UEdGraphPin* InPin = nullptr;
                for (UEdGraphPin* P : EndNode->Pins)
                {
                    if (P && P->Direction == EGPD_Input) { InPin = P; break; }
                }
                if (InPin && ParentGraph->GetSchema()->TryCreateConnection(FromPin, InPin))
                {
                    ParentGraph->NotifyGraphChanged();
                }
            }

            EndNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return EndNode;
        }
    };

    struct FDialogueAddEntryPointAction : public FEdGraphSchemaAction
    {
        FDialogueAddEntryPointAction(
            const FText& InCategory,
            const FText& InMenuDesc,
            const FText& InToolTip,
            int32 InGrouping)
            : FEdGraphSchemaAction(InCategory, InMenuDesc, InToolTip, InGrouping) {}

        virtual UEdGraphNode* PerformAction(
            UEdGraph* ParentGraph,
            UEdGraphPin* FromPin,
            const FVector2D Location,
            bool bSelectNewNode) override
        {
            if (!ParentGraph) return nullptr;

            const FScopedTransaction Transaction(
                NSLOCTEXT("DialogueGraph", "AddEntryPoint", "Add Entry Point"));

            ParentGraph->Modify();

            UDialogueEntryGizmo* EntryNode = NewObject<UDialogueEntryGizmo>(
                ParentGraph,
                UDialogueEntryGizmo::StaticClass(),
                NAME_None,
                RF_Transactional);

            if (!EntryNode) return nullptr;

            EntryNode->Modify();
            EntryNode->EntryPointId = FName(TEXT("Return"));
            EntryNode->NodePosX = (int32)Location.X;
            EntryNode->NodePosY = (int32)Location.Y;

            ParentGraph->AddNode(EntryNode, true, bSelectNewNode);
            EntryNode->AllocateDefaultPins();

            EntryNode->PostEditChange();
            ParentGraph->NotifyGraphChanged();

            return EntryNode;
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

static bool IsDialogueFlowLikePin(const UEdGraphPin* Pin)
{
    if (!Pin)
    {
        return false;
    }

    const FName Category = Pin->PinType.PinCategory;
    // Accept legacy categories to keep old nodes connectable after schema changes.
    return Category == DialogueGraphPins::Flow
        || Category == FName(TEXT("Flow"))
        || Category.IsNone();
}

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
    const FText Category = LOCTEXT("DialogueCategory", "Dialogue");

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueNewNodeAction>(
        Category,
        LOCTEXT("AddDialogueNode", "Add Dialogue Node"),
        LOCTEXT("AddDialogueNodeTooltip", "Creates a dialogue node. Connect the Start gizmo to a node's From pin to set where the dialogue begins."),
        0));

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueAddEndNodeAction>(
        Category,
        LOCTEXT("AddEndNode", "Add End Node"),
        LOCTEXT("AddEndNodeTooltip", "Creates an End node. Connect choice pins here to mark where the dialogue ends."),
        1));

    ContextMenuBuilder.AddAction(MakeShared<DialogueSchemaActions::FDialogueAddEntryPointAction>(
        Category,
        LOCTEXT("AddEntryPoint", "Add Entry Point"),
        LOCTEXT("AddEntryPointTooltip", "Creates an alternate entry point. Set its ID (e.g. Return, Continue) and connect to a dialogue node. Use Start(Asset, EntryPointId) to begin from this node."),
        2));
}

const FPinConnectionResponse UDialogueGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const
{
    if (!A || !B)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins"));

    if (!IsDialogueFlowLikePin(A) || !IsDialogueFlowLikePin(B))
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Only dialogue flow connections allowed"));

    if (A->Direction == B->Direction)
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pins must be opposite directions"));

    const UEdGraphPin* OutPin = (A->Direction == EGPD_Output) ? A : B;
    const UEdGraphPin* InPin  = (A->Direction == EGPD_Input)  ? A : B;

    // Output must be from Dialogue node or Start gizmo
    UEdGraphNode* OutOwner = OutPin->GetOwningNode();
    if (!OutOwner) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid output"));

    // Input must be Dialogue node or End gizmo (Start output -> Dialogue only; Dialogue output -> Dialogue or End)
    UEdGraphNode* InOwner = InPin->GetOwningNode();
    if (!InOwner) return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid input"));

    if (Cast<UDialogueStartGizmo>(OutOwner))
    {
        if (!Cast<UDialogueGraphNode>(InOwner))
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Start must connect to a Dialogue node"));
    }
    else if (Cast<UDialogueEntryGizmo>(OutOwner))
    {
        if (!Cast<UDialogueGraphNode>(InOwner))
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Entry Point must connect to a Dialogue node"));
    }
    else if (Cast<UDialogueGraphNode>(OutOwner))
    {
        if (!Cast<UDialogueGraphNode>(InOwner) && !Cast<UDialogueEndGizmo>(InOwner))
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Dialogue output must connect to a Dialogue node or End node"));
    }
    else if (Cast<UDialogueEndGizmo>(OutOwner))
    {
        // End "NextStart" output -> Entry Point "NextStart" input: "when you exit via this End, next time start here"
        if (!Cast<UDialogueEntryGizmo>(InOwner))
            return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("End output must connect to an Entry Point (sets \"next start here\")"));
    }
    else
    {
        return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Output must be from Start, Dialogue, or End node"));
    }

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
        UDialogueGraphNode* DNode = Context->Node ? Cast<UDialogueGraphNode>(const_cast<UEdGraphNode*>(Context->Node.Get())) : nullptr;

        if (DNode)
        {
            // Collect all selected dialogue nodes (or just the right-clicked one if none selected)
            TArray<TWeakObjectPtr<UDialogueGraphNode>> SelectedDialogueNodes;
            if (Context->Graph)
            {
                if (TSharedPtr<SGraphEditor> GraphEd = SGraphEditor::FindGraphEditorForGraph(Context->Graph))
                {
                    const FGraphPanelSelectionSet& Selected = GraphEd->GetSelectedNodes();
                    for (UObject* Obj : Selected)
                    {
                        if (UDialogueGraphNode* DN = Cast<UDialogueGraphNode>(Obj))
                        {
                            SelectedDialogueNodes.Add(DN);
                        }
                    }
                }
            }
            if (SelectedDialogueNodes.Num() == 0)
            {
                SelectedDialogueNodes.Add(DNode);
            }

            Section.AddSubMenu(
                "DialogueChangeColor",
                FText::FromString("Change Color"),
                SelectedDialogueNodes.Num() > 1
                    ? FText::Format(INVTEXT("Set color for {0} selected nodes"), FText::AsNumber(SelectedDialogueNodes.Num()))
                    : FText::FromString(TEXT("Set node color for visual organization of dialogue paths")),
                FNewToolMenuChoice(FNewToolMenuDelegate::CreateLambda([SelectedDialogueNodes](UToolMenu* SubMenu)
                {
                    if (!SubMenu || SelectedDialogueNodes.Num() == 0) return;

                    struct FColorOption { FName Id; FText Label; FLinearColor Color; };
                    const TArray<FColorOption> Colors = {
                        { "Default", FText::FromString("Default"), FLinearColor(0.05f, 0.05f, 0.05f, 0.8f) },
                        { "Red", FText::FromString("Red"), FLinearColor(0.5f, 0.15f, 0.15f, 0.9f) },
                        { "Blue", FText::FromString("Blue"), FLinearColor(0.15f, 0.2f, 0.5f, 0.9f) },
                        { "Green", FText::FromString("Green"), FLinearColor(0.1f, 0.4f, 0.15f, 0.9f) },
                        { "Yellow", FText::FromString("Yellow"), FLinearColor(0.5f, 0.45f, 0.1f, 0.9f) },
                        { "Orange", FText::FromString("Orange"), FLinearColor(0.55f, 0.3f, 0.1f, 0.9f) },
                        { "Purple", FText::FromString("Purple"), FLinearColor(0.35f, 0.15f, 0.5f, 0.9f) },
                        { "Cyan", FText::FromString("Cyan"), FLinearColor(0.1f, 0.4f, 0.45f, 0.9f) },
                        { "Pink", FText::FromString("Pink"), FLinearColor(0.5f, 0.2f, 0.4f, 0.9f) },
                    };

                    FToolMenuSection& ColorSection = SubMenu->AddSection("DialogueColors", FText::FromString("Presets"));
                    for (const FColorOption& Opt : Colors)
                    {
                        TSharedRef<SWidget> ColorRow = SNew(SHorizontalBox)
                            + SHorizontalBox::Slot()
                            .AutoWidth()
                            .Padding(0, 0, 8, 0)
                            .VAlign(VAlign_Center)
                            [
                                SNew(SColorBlock)
                                .Color(Opt.Color)
                                .Size(FVector2D(14, 14))
                                .AlphaDisplayMode(EColorBlockAlphaDisplayMode::Ignore)
                            ]
                            + SHorizontalBox::Slot()
                            .FillWidth(1.f)
                            .VAlign(VAlign_Center)
                            [
                                SNew(STextBlock).Text(Opt.Label)
                            ];
                        ColorSection.AddEntry(FToolMenuEntry::InitMenuEntry(
                            Opt.Id,
                            FUIAction(FExecuteAction::CreateLambda([SelectedDialogueNodes, Opt]()
                            {
                                const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "ChangeNodeColor", "Change Node Color"));
                                UEdGraph* Graph = nullptr;
                                for (const TWeakObjectPtr<UDialogueGraphNode>& WeakN : SelectedDialogueNodes)
                                {
                                    if (UDialogueGraphNode* N = WeakN.Get())
                                    {
                                        if (!Graph) { Graph = N->GetGraph(); if (Graph) Graph->Modify(); }
                                        N->Modify();
                                        N->NodeColor = Opt.Color;
                                    }
                                }
                                if (Graph) Graph->NotifyGraphChanged();
                            })),
                            ColorRow
                        ));
                    }

                    ColorSection.AddSeparator("CustomSep");
                    ColorSection.AddMenuEntry(
                        "DialogueCustomColor",
                        FText::FromString("Custom..."),
                        FText::FromString("Open RGB color picker to choose any color"),
                        FSlateIcon(),
                        FUIAction(FExecuteAction::CreateLambda([SelectedDialogueNodes]()
                        {
                            if (SelectedDialogueNodes.Num() == 0) return;
                            FLinearColor InitialColor = FLinearColor(0.05f, 0.05f, 0.05f, 0.8f);
                            if (UDialogueGraphNode* First = SelectedDialogueNodes[0].Get())
                            {
                                InitialColor = First->NodeColor;
                            }
                            FColorPickerArgs Args;
                            Args.bUseAlpha = true;
                            Args.InitialColor = InitialColor;
                            Args.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda([SelectedDialogueNodes](FLinearColor NewColor)
                            {
                                const FScopedTransaction Tx(NSLOCTEXT("DialogueGraph", "ChangeNodeColor", "Change Node Color"));
                                UEdGraph* Graph = nullptr;
                                for (const TWeakObjectPtr<UDialogueGraphNode>& WeakN : SelectedDialogueNodes)
                                {
                                    if (UDialogueGraphNode* N = WeakN.Get())
                                    {
                                        if (!Graph) { Graph = N->GetGraph(); if (Graph) Graph->Modify(); }
                                        N->Modify();
                                        N->NodeColor = NewColor;
                                    }
                                }
                                if (Graph) Graph->NotifyGraphChanged();
                            });
                            OpenColorPicker(Args);
                        }))
                    );
                })),
                false
            );
        }

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
        UEdGraphNode* ToNode = OutPin->LinkedTo[0]->GetOwningNode();
        if (UDialogueGraphNode* DNode = Cast<UDialogueGraphNode>(ToNode))
        {
            NewNext = DNode->NodeId;
        }
        // Connected to End gizmo: NewNext stays NAME_None (end of dialogue)
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

#else

void UDialogueGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
}

const FPinConnectionResponse UDialogueGraphSchema::CanCreateConnection(
    const UEdGraphPin* A, const UEdGraphPin* B) const
{
    return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FText::GetEmpty());
}

void UDialogueGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
}

bool UDialogueGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
    return false;
}

void UDialogueGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotif) const
{
}

void UDialogueGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
}

#endif

#undef LOCTEXT_NAMESPACE
