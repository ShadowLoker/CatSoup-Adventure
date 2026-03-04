#include "SDialogueGraphNode.h"

#include "Dialogue/Graph/DialogueGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
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
        .Padding(10)
        [
            SNew(SHorizontalBox)

            // LEFT: Input pins
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(LeftNodeBox, SVerticalBox)
            ]

            // CENTER: Speaker + Text
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

            // RIGHT: Output pins
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
                SAssignNew(RightNodeBox, SVerticalBox)
            ]
        ]
    ];

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

    DNode->NodeData.Outputs[NewIndex].Text =
        FText::FromString(FString::Printf(TEXT("Option %d"), NewIndex + 1));
    DNode->NodeData.Outputs[NewIndex].NextNodeId = NAME_None;

    DNode->ReconstructNode();
    if (UEdGraph* G = DNode->GetGraph()) G->NotifyGraphChanged();

    return FReply::Handled();
}