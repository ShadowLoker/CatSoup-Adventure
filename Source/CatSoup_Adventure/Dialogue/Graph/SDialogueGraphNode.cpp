#include "SDialogueGraphNode.h"

#include "Dialogue/Graph/DialogueGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"

void SDialogueGraphNode::Construct(const FArguments& InArgs, UDialogueGraphNode* InNode)
{
    DNode = InNode;
    GraphNode = InNode;
    SetCursor(EMouseCursor::CardinalCross);
    UpdateGraphNode();
}

void SDialogueGraphNode::UpdateGraphNode()
{
    InputPins.Empty();
    OutputPins.Empty();

    RightNodeBox.Reset();
    LeftNodeBox.Reset();

    this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

    TSharedPtr<SVerticalBox> MainVBox;

    // Core layout: [Left Pins] [Center Content] [Right Pins]
    const FLinearColor NodeColor = DNode ? DNode->NodeColor : FLinearColor(0.05f, 0.05f, 0.05f, 0.8f);
    this->GetOrAddSlot(ENodeZone::Center)
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
        .BorderBackgroundColor(NodeColor)
        .Padding(10)
        [
            SNew(SHorizontalBox)

            // LEFT: Input pins
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(LeftNodeBox, SVerticalBox)
            ]

            // CENTER: Speaker + Text only
            + SHorizontalBox::Slot()
            .FillWidth(1.f)
            .Padding(12, 0)
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 6)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("SPEAKER")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 12)
                [
                    SNew(SEditableTextBox)
                    .Text(TAttribute<FText>::CreateLambda([WeakNode = TWeakObjectPtr<UDialogueGraphNode>(DNode)]()
                    {
                        UDialogueGraphNode* Node = WeakNode.Get();
                        return Node ? FText::FromString(Node->NodeData.SpeakerId.ToString()) : FText::GetEmpty();
                    }))
                    .OnTextCommitted(FOnTextCommitted::CreateLambda([WeakNode = TWeakObjectPtr<UDialogueGraphNode>(DNode)](const FText& NewText, ETextCommit::Type)
                    {
                        if (UDialogueGraphNode* Node = WeakNode.Get())
                        {
                            Node->Modify();
                            Node->NodeData.SpeakerId = FName(*NewText.ToString());
                            if (UEdGraph* G = Node->GetGraph()) G->NotifyGraphChanged();
                        }
                    }))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 6)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("TEXT")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SBox)
                    .MinDesiredHeight(80.f)
                    [
                        SNew(SMultiLineEditableTextBox)
                        .Text(TAttribute<FText>::CreateLambda([WeakNode = TWeakObjectPtr<UDialogueGraphNode>(DNode)]()
                        {
                            UDialogueGraphNode* Node = WeakNode.Get();
                            return Node ? Node->NodeData.Text : FText::GetEmpty();
                        }))
                        .OnTextCommitted(FOnTextCommitted::CreateLambda([WeakNode = TWeakObjectPtr<UDialogueGraphNode>(DNode)](const FText& NewText, ETextCommit::Type)
                        {
                            if (UDialogueGraphNode* Node = WeakNode.Get())
                            {
                                Node->Modify();
                                Node->NodeData.Text = NewText;
                                Node->ReconstructNode();
                                if (UEdGraph* G = Node->GetGraph()) G->NotifyGraphChanged();
                            }
                        }))
                    ]
                ]
            ]

            // RIGHT: Choices label + output labels (editable) + pins + Add Output button below
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, 4.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("Choices")))
                    .Visibility(DNode ? EVisibility::Visible : EVisibility::Collapsed)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    // Pins first (left) so they're prominent and easy to drag from
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .MinWidth(28.f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        [
                            SAssignNew(RightNodeBox, SVerticalBox)
                        ]
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(6, 0, 0, 0)
                    [
                        SNew(SBox)
                        .Visibility(DNode ? EVisibility::Visible : EVisibility::Collapsed)
                        [
                            SAssignNew(OutputLabelsBox, SVerticalBox)
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(4.f, 6.f)
                [
                    SNew(SButton)
                    .Text(FText::FromString(TEXT("+ Add Output")))
                    .OnClicked(this, &SDialogueGraphNode::OnAddOutputClicked)
                    .Visibility(DNode ? EVisibility::Visible : EVisibility::Collapsed)
                ]
            ]
        ]
    ];

    // Populate output labels (editable) for Dialogue nodes
    // Use TWeakObjectPtr to avoid capturing 'this' - when the graph refreshes after ReconstructNode,
    // the Slate widget may be destroyed while callbacks are still pending, causing EXCEPTION_ACCESS_VIOLATION.
    if (OutputLabelsBox.IsValid() && DNode)
    {
        TWeakObjectPtr<UDialogueGraphNode> WeakNode(DNode);
        for (int32 i = 0; i < DNode->NodeData.Outputs.Num(); i++)
        {
            const int32 Index = i;
            OutputLabelsBox->AddSlot()
                .AutoHeight()
                .Padding(0, 2.f)
                [
                    SNew(SEditableTextBox)
                    .MinDesiredWidth(60.f)
                    .Text(TAttribute<FText>::CreateLambda([WeakNode, Index]()
                    {
                        UDialogueGraphNode* Node = WeakNode.Get();
                        if (!Node || !Node->NodeData.Outputs.IsValidIndex(Index)) return FText::GetEmpty();
                        return Node->NodeData.Outputs[Index].Text;
                    }))
                    .OnTextCommitted(FOnTextCommitted::CreateLambda([WeakNode, Index](const FText& NewText, ETextCommit::Type)
                    {
                        UDialogueGraphNode* Node = WeakNode.Get();
                        if (!Node || !Node->NodeData.Outputs.IsValidIndex(Index)) return;
                        Node->Modify();
                        Node->NodeData.Outputs[Index].Text = NewText;
                        Node->ReconstructNode();
                        if (UEdGraph* G = Node->GetGraph()) G->NotifyGraphChanged();
                    }))
                ];
        }
    }

    CreatePinWidgets();
}

void SDialogueGraphNode::CreatePinWidgets()
{
    for (UEdGraphPin* CurPin : GraphNode->Pins)
    {
        if (!CurPin) continue;

        TSharedPtr<SGraphPin> NewPin = CreatePinWidget(CurPin);
        if (!NewPin.IsValid()) continue;

        AddPin(NewPin.ToSharedRef());
    }
}

FReply SDialogueGraphNode::OnAddOutputClicked()
{
    if (!DNode) return FReply::Handled();

    DNode->Modify();

    DNode->NodeData.Outputs.AddDefaulted();
    const int32 NewIndex = DNode->NodeData.Outputs.Num() - 1;

    DNode->NodeData.Outputs[NewIndex].Text = FText::FromString(TEXT("End"));
    DNode->NodeData.Outputs[NewIndex].NextNodeId = NAME_None;

    DNode->ReconstructNode();
    if (UEdGraph* G = DNode->GetGraph()) G->NotifyGraphChanged();

    // Rebuild this slate widget so new pins show up immediately
    UpdateGraphNode();

    return FReply::Handled();
}
