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
    this->GetOrAddSlot(ENodeZone::Center)
    [
        SNew(SBorder)
        .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.8f))
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
                    .Text(this, &SDialogueGraphNode::GetSpeakerText)
                    .OnTextCommitted(this, &SDialogueGraphNode::OnSpeakerCommitted)
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
                        .Text(this, &SDialogueGraphNode::GetLineText)
                        .OnTextCommitted(this, &SDialogueGraphNode::OnLineCommitted)
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
                    .Visibility(DNode && DNode->NodeType == EDialogueGraphNodeType::Dialogue ? EVisibility::Visible : EVisibility::Collapsed)
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0, 0, 6, 0)
                [
                    SNew(SBox)
                    .Visibility(DNode && DNode->NodeType == EDialogueGraphNodeType::Dialogue ? EVisibility::Visible : EVisibility::Collapsed)
                    [
                        SAssignNew(OutputLabelsBox, SVerticalBox)
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(RightNodeBox, SVerticalBox)
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    .Padding(4.f, 6.f)
                    [
                        SNew(SButton)
                        .Text(FText::FromString(TEXT("+ Add Output")))
                        .OnClicked(this, &SDialogueGraphNode::OnAddOutputClicked)
                        .Visibility(DNode && DNode->NodeType == EDialogueGraphNodeType::Dialogue ? EVisibility::Visible : EVisibility::Collapsed)
                    ]
                ]
                ]
            ]
        ]
    ];

    // Populate output labels (editable) for Dialogue nodes
    if (OutputLabelsBox.IsValid() && DNode && DNode->NodeType == EDialogueGraphNodeType::Dialogue)
    {
        for (int32 i = 0; i < DNode->NodeData.Outputs.Num(); i++)
        {
            const int32 Index = i;
            OutputLabelsBox->AddSlot()
                .AutoHeight()
                .Padding(0, 2.f)
                [
                    SNew(SEditableTextBox)
                    .MinDesiredWidth(60.f)
                    .Text(TAttribute<FText>::CreateLambda([this, Index]() { return GetOutputText(Index); }))
                    .OnTextCommitted(FOnTextCommitted::CreateLambda([this, Index](const FText& T, ETextCommit::Type C) { OnOutputTextCommitted(T, C, Index); }))
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

FText SDialogueGraphNode::GetSpeakerText() const
{
    if (!DNode) return FText::GetEmpty();
    return FText::FromString(DNode->NodeData.SpeakerId.ToString());
}

void SDialogueGraphNode::OnSpeakerCommitted(const FText& NewText, ETextCommit::Type)
{
    if (!DNode) return;
    DNode->Modify();
    DNode->NodeData.SpeakerId = FName(*NewText.ToString());
    if (UEdGraph* G = DNode->GetGraph()) G->NotifyGraphChanged();
}

FText SDialogueGraphNode::GetLineText() const
{
    if (!DNode) return FText::GetEmpty();
    return DNode->NodeData.Text;
}

void SDialogueGraphNode::OnLineCommitted(const FText& NewText, ETextCommit::Type)
{
    if (!DNode) return;
    DNode->Modify();
    DNode->NodeData.Text = NewText;
    DNode->ReconstructNode(); // updates title/pin labels if you want
    if (UEdGraph* G = DNode->GetGraph()) G->NotifyGraphChanged();
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

FText SDialogueGraphNode::GetOutputText(int32 Index) const
{
    if (!DNode || !DNode->NodeData.Outputs.IsValidIndex(Index))
        return FText::GetEmpty();
    return DNode->NodeData.Outputs[Index].Text;
}

void SDialogueGraphNode::OnOutputTextCommitted(const FText& NewText, ETextCommit::Type, int32 Index)
{
    if (!DNode || !DNode->NodeData.Outputs.IsValidIndex(Index))
        return;
    DNode->Modify();
    DNode->NodeData.Outputs[Index].Text = NewText;
    DNode->ReconstructNode();
    if (UEdGraph* G = DNode->GetGraph())
        G->NotifyGraphChanged();
    UpdateGraphNode();
}
